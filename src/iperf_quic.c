#include "iperf_config.h"

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_quic.h"
#include "iperf_util.h"
#include "net.h"

#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_MSQUIC
#include "msquic.h"

struct quic_recv_chunk {
    uint8_t *data;
    size_t len;
    size_t off;
    struct quic_recv_chunk *next;
};

struct iperf_quic_stream_ctx {
    HQUIC stream;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    struct quic_recv_chunk *head;
    struct quic_recv_chunk *tail;
    size_t queued_len;
    int recv_shutdown;
    struct iperf_test *test;
};

struct quic_stream_map {
    int id;
    struct iperf_quic_stream_ctx *ctx;
    struct quic_stream_map *next;
};

struct quic_accept_node {
    struct iperf_quic_stream_ctx *ctx;
    struct quic_accept_node *next;
};

struct iperf_quic_context {
    const QUIC_API_TABLE *api;
    HQUIC registration;
    HQUIC configuration;
    HQUIC listener;
    HQUIC connection;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int ready;
    int is_server;
    int shutdown_complete;
    int notify_fds[2];
    int next_stream_id;
    struct quic_stream_map *map;
    struct quic_accept_node *accept_head;
    struct quic_accept_node *accept_tail;
};

static const QUIC_BUFFER kQuicAlpn = { sizeof("iperf") - 1, (uint8_t *)"iperf" };
static const QUIC_REGISTRATION_CONFIG kQuicRegConfig = { "iperf3", QUIC_EXECUTION_PROFILE_LOW_LATENCY };
static const QUIC_API_TABLE *MsQuic;

static void
quic_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void
quic_signal_notify(struct iperf_quic_context *ctx)
{
    if (ctx->notify_fds[1] >= 0) {
        uint8_t b = 1;
        (void)write(ctx->notify_fds[1], &b, 1);
    }
}

static struct iperf_quic_stream_ctx *
quic_stream_ctx_create(HQUIC stream, struct iperf_test *test)
{
    struct iperf_quic_stream_ctx *sctx = calloc(1, sizeof(*sctx));
    if (!sctx)
        return NULL;
    sctx->stream = stream;
    sctx->test = test;
    pthread_mutex_init(&sctx->lock, NULL);
    pthread_cond_init(&sctx->cond, NULL);
    return sctx;
}

static void
quic_stream_ctx_free(struct iperf_quic_stream_ctx *sctx)
{
    struct quic_recv_chunk *c = sctx->head;
    while (c) {
        struct quic_recv_chunk *next = c->next;
        free(c->data);
        free(c);
        c = next;
    }
    pthread_mutex_destroy(&sctx->lock);
    pthread_cond_destroy(&sctx->cond);
    free(sctx);
}

static void
quic_map_insert(struct iperf_quic_context *ctx, int id, struct iperf_quic_stream_ctx *sctx)
{
    struct quic_stream_map *node = calloc(1, sizeof(*node));
    if (!node)
        return;
    node->id = id;
    node->ctx = sctx;
    node->next = ctx->map;
    ctx->map = node;
}

static struct iperf_quic_stream_ctx *
quic_map_take(struct iperf_quic_context *ctx, int id)
{
    struct quic_stream_map *prev = NULL;
    struct quic_stream_map *cur = ctx->map;
    while (cur) {
        if (cur->id == id) {
            if (prev)
                prev->next = cur->next;
            else
                ctx->map = cur->next;
            struct iperf_quic_stream_ctx *sctx = cur->ctx;
            free(cur);
            return sctx;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

static void
quic_accept_enqueue(struct iperf_quic_context *ctx, struct iperf_quic_stream_ctx *sctx)
{
    struct quic_accept_node *node = calloc(1, sizeof(*node));
    if (!node)
        return;
    node->ctx = sctx;
    if (!ctx->accept_tail) {
        ctx->accept_head = ctx->accept_tail = node;
    } else {
        ctx->accept_tail->next = node;
        ctx->accept_tail = node;
    }
    quic_signal_notify(ctx);
}

static struct iperf_quic_stream_ctx *
quic_accept_dequeue(struct iperf_quic_context *ctx)
{
    struct quic_accept_node *node = ctx->accept_head;
    if (!node)
        return NULL;
    ctx->accept_head = node->next;
    if (!ctx->accept_head)
        ctx->accept_tail = NULL;
    struct iperf_quic_stream_ctx *sctx = node->ctx;
    free(node);
    return sctx;
}

static QUIC_STATUS QUIC_API
quic_stream_callback(HQUIC Stream, void *Context, QUIC_STREAM_EVENT *Event)
{
    struct iperf_quic_stream_ctx *sctx = (struct iperf_quic_stream_ctx *)Context;
    switch (Event->Type) {
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        if (Event->SEND_COMPLETE.ClientContext)
            free(Event->SEND_COMPLETE.ClientContext);
        break;
    case QUIC_STREAM_EVENT_RECEIVE: {
        uint64_t total = 0;
        for (uint32_t i = 0; i < Event->RECEIVE.BufferCount; ++i) {
            const QUIC_BUFFER *b = &Event->RECEIVE.Buffers[i];
            struct quic_recv_chunk *chunk = calloc(1, sizeof(*chunk));
            if (!chunk)
                break;
            chunk->data = malloc(b->Length);
            if (!chunk->data) {
                free(chunk);
                break;
            }
            memcpy(chunk->data, b->Buffer, b->Length);
            chunk->len = b->Length;
            pthread_mutex_lock(&sctx->lock);
            if (!sctx->tail) {
                sctx->head = sctx->tail = chunk;
            } else {
                sctx->tail->next = chunk;
                sctx->tail = chunk;
            }
            sctx->queued_len += chunk->len;
            pthread_cond_signal(&sctx->cond);
            pthread_mutex_unlock(&sctx->lock);
            total += b->Length;
        }
        if (total > 0) {
            MsQuic->StreamReceiveComplete(Stream, total);
            MsQuic->StreamReceiveSetEnabled(Stream, TRUE);
        }
        break;
    }
    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        pthread_mutex_lock(&sctx->lock);
        sctx->recv_shutdown = 1;
        pthread_cond_broadcast(&sctx->cond);
        pthread_mutex_unlock(&sctx->lock);
        break;
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
            MsQuic->StreamClose(Stream);
        }
        break;
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

static QUIC_STATUS QUIC_API
quic_connection_callback(HQUIC Connection, void *Context, QUIC_CONNECTION_EVENT *Event)
{
    struct iperf_test *test = (struct iperf_test *)Context;
    struct iperf_quic_context *ctx = test->quic_ctx;
    switch (Event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        pthread_mutex_lock(&ctx->lock);
        ctx->ready = 1;
        pthread_cond_broadcast(&ctx->cond);
        pthread_mutex_unlock(&ctx->lock);
        break;
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
        HQUIC stream = Event->PEER_STREAM_STARTED.Stream;
        struct iperf_quic_stream_ctx *sctx = quic_stream_ctx_create(stream, test);
        if (!sctx)
            break;
        ctx->api->SetCallbackHandler(stream, (void *)quic_stream_callback, sctx);
        ctx->api->StreamReceiveSetEnabled(stream, TRUE);
        pthread_mutex_lock(&ctx->lock);
        quic_accept_enqueue(ctx, sctx);
        pthread_mutex_unlock(&ctx->lock);
        break;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        pthread_mutex_lock(&ctx->lock);
        ctx->ready = 0;
        ctx->shutdown_complete = 1;
        pthread_cond_broadcast(&ctx->cond);
        pthread_mutex_unlock(&ctx->lock);
        quic_signal_notify(ctx);
        break;
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

static QUIC_STATUS QUIC_API
quic_listener_callback(HQUIC Listener, void *Context, QUIC_LISTENER_EVENT *Event)
{
    struct iperf_test *test = (struct iperf_test *)Context;
    struct iperf_quic_context *ctx = test->quic_ctx;
    (void)Listener;
    if (Event->Type == QUIC_LISTENER_EVENT_NEW_CONNECTION) {
        ctx->connection = Event->NEW_CONNECTION.Connection;
        ctx->api->SetCallbackHandler(ctx->connection, (void *)quic_connection_callback, test);
        ctx->api->ConnectionSetConfiguration(ctx->connection, ctx->configuration);
    }
    return QUIC_STATUS_SUCCESS;
}

static int
quic_open_api(struct iperf_quic_context *ctx)
{
    QUIC_STATUS status = MsQuicOpen2(&ctx->api);
    if (QUIC_FAILED(status))
        return -1;
    MsQuic = ctx->api;
    status = ctx->api->RegistrationOpen(&kQuicRegConfig, &ctx->registration);
    if (QUIC_FAILED(status))
        return -1;
    return 0;
}

static int
quic_open_configuration(struct iperf_test *test, struct iperf_quic_context *ctx)
{
    QUIC_SETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    if (test->settings->idle_timeout > 0) {
        settings.IdleTimeoutMs = (uint64_t)test->settings->idle_timeout * 1000;
        settings.IsSet.IdleTimeoutMs = 1;
    }
    /* Increase flow control and receive buffers for throughput tests. */
    uint32_t quic_buf = test->quic_buf_size ? test->quic_buf_size : (32 * 1024 * 1024);
    settings.StreamRecvWindowDefault = quic_buf;
    settings.StreamRecvWindowBidiLocalDefault = quic_buf;
    settings.StreamRecvWindowBidiRemoteDefault = quic_buf;
    settings.StreamRecvWindowUnidiDefault = quic_buf;
    settings.StreamRecvBufferDefault = quic_buf;
    settings.ConnFlowControlWindow = (uint64_t)quic_buf * 4;
    settings.SendBufferingEnabled = 1;
    settings.IsSet.StreamRecvWindowDefault = 1;
    settings.IsSet.StreamRecvWindowBidiLocalDefault = 1;
    settings.IsSet.StreamRecvWindowBidiRemoteDefault = 1;
    settings.IsSet.StreamRecvWindowUnidiDefault = 1;
    settings.IsSet.StreamRecvBufferDefault = 1;
    settings.IsSet.ConnFlowControlWindow = 1;
    settings.IsSet.SendBufferingEnabled = 1;
    if (test->num_streams > 0) {
        settings.PeerBidiStreamCount = (uint16_t)(test->num_streams * (test->bidirectional ? 2 : 1));
        if (settings.PeerBidiStreamCount == 0)
            settings.PeerBidiStreamCount = 1;
        settings.IsSet.PeerBidiStreamCount = 1;
    }

    QUIC_STATUS status = ctx->api->ConfigurationOpen(
        ctx->registration,
        &kQuicAlpn,
        1,
        &settings,
        sizeof(settings),
        NULL,
        &ctx->configuration);
    if (QUIC_FAILED(status)) {
        iperf_err(test, "MsQuic ConfigurationOpen failed, 0x%x", status);
        return -1;
    }

    QUIC_CREDENTIAL_CONFIG cred;
    memset(&cred, 0, sizeof(cred));
    if (ctx->is_server) {
        if (test->quic_p12_file) {
            FILE *fp = fopen(test->quic_p12_file, "rb");
            if (!fp) {
                i_errno = IEFILE;
                return -1;
            }
            if (fseek(fp, 0, SEEK_END) != 0) {
                fclose(fp);
                i_errno = IEFILE;
                return -1;
            }
            long sz = ftell(fp);
            if (sz <= 0) {
                fclose(fp);
                i_errno = IEFILE;
                return -1;
            }
            if (fseek(fp, 0, SEEK_SET) != 0) {
                fclose(fp);
                i_errno = IEFILE;
                return -1;
            }
            uint8_t *blob = malloc((size_t)sz);
            if (!blob) {
                fclose(fp);
                i_errno = IEFILE;
                return -1;
            }
            if (fread(blob, 1, (size_t)sz, fp) != (size_t)sz) {
                fclose(fp);
                free(blob);
                i_errno = IEFILE;
                return -1;
            }
            fclose(fp);

            QUIC_CERTIFICATE_PKCS12 pkcs12;
            pkcs12.Asn1Blob = blob;
            pkcs12.Asn1BlobLength = (uint32_t)sz;
            pkcs12.PrivateKeyPassword = test->quic_p12_password;

            cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
            cred.CertificatePkcs12 = &pkcs12;
            cred.Flags = QUIC_CREDENTIAL_FLAG_USE_PORTABLE_CERTIFICATES;

            status = ctx->api->ConfigurationLoadCredential(ctx->configuration, &cred);
            free(blob);
            if (QUIC_FAILED(status)) {
                iperf_err(test, "MsQuic ConfigurationLoadCredential failed, 0x%x", status);
                return -1;
            }
        } else {
            if (!test->quic_cert_file || !test->quic_key_file) {
                i_errno = IEFILE;
                return -1;
            }
            cred.Flags = QUIC_CREDENTIAL_FLAG_USE_PORTABLE_CERTIFICATES;
            if (test->quic_key_password) {
                QUIC_CERTIFICATE_FILE_PROTECTED cert = { test->quic_cert_file, test->quic_key_file, test->quic_key_password };
                cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE_PROTECTED;
                cred.CertificateFileProtected = &cert;
            } else {
                QUIC_CERTIFICATE_FILE cert = { test->quic_cert_file, test->quic_key_file };
                cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
                cred.CertificateFile = &cert;
            }
            status = ctx->api->ConfigurationLoadCredential(ctx->configuration, &cred);
            if (QUIC_FAILED(status)) {
                iperf_err(test, "MsQuic ConfigurationLoadCredential failed, 0x%x", status);
                return -1;
            }
        }
    } else {
        cred.Type = QUIC_CREDENTIAL_TYPE_NONE;
        cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
        status = ctx->api->ConfigurationLoadCredential(ctx->configuration, &cred);
        if (QUIC_FAILED(status)) {
            iperf_err(test, "MsQuic ConfigurationLoadCredential failed, 0x%x", status);
            return -1;
        }
    }

    return 0;
}

static int
quic_ctx_init(struct iperf_test *test, int is_server)
{
    if (test->quic_ctx)
        return 0;
    struct iperf_quic_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return -1;
    ctx->is_server = is_server;
    ctx->notify_fds[0] = -1;
    ctx->notify_fds[1] = -1;
    ctx->shutdown_complete = 0;
    pthread_mutex_init(&ctx->lock, NULL);
    pthread_cond_init(&ctx->cond, NULL);
    if (quic_open_api(ctx) < 0)
        goto fail;
    if (quic_open_configuration(test, ctx) < 0)
        goto fail;

    if (pipe(ctx->notify_fds) != 0)
        goto fail;
    quic_set_nonblocking(ctx->notify_fds[0]);
    quic_set_nonblocking(ctx->notify_fds[1]);

    test->quic_ctx = ctx;
    return 0;

fail:
    test->quic_ctx = ctx;
    iperf_quic_free_test(test);
    return -1;
}

static void
quic_close_stream_handle(struct iperf_quic_context *ctx, struct iperf_quic_stream_ctx *sctx)
{
    if (!sctx)
        return;
    if (sctx->stream) {
        ctx->api->StreamShutdown(sctx->stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
        ctx->api->StreamClose(sctx->stream);
        sctx->stream = NULL;
    }
}

int
iperf_quic_init(struct iperf_test *test)
{
    (void)test;
    return 0;
}

int
iperf_quic_listen(struct iperf_test *test)
{
    if (quic_ctx_init(test, 1) < 0) {
        if (i_errno == 0)
            i_errno = IELISTEN;
        return -1;
    }
    struct iperf_quic_context *ctx = test->quic_ctx;
    QUIC_STATUS status;

    if (QUIC_FAILED(status = ctx->api->ListenerOpen(ctx->registration, quic_listener_callback, test, &ctx->listener))) {
        iperf_err(test, "MsQuic ListenerOpen failed, 0x%x", status);
        i_errno = IELISTEN;
        return -1;
    }

    QUIC_ADDR addr;
    memset(&addr, 0, sizeof(addr));
    int family = 0;
    if (test->bind_address) {
        struct sockaddr_in *v4 = (struct sockaddr_in *)&addr;
        if (inet_pton(AF_INET, test->bind_address, &v4->sin_addr) == 1) {
            family = AF_INET;
            v4->sin_family = AF_INET;
        } else {
            struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&addr;
            if (inet_pton(AF_INET6, test->bind_address, &v6->sin6_addr) == 1) {
                family = AF_INET6;
                v6->sin6_family = AF_INET6;
            }
        }
    }

    if (family == 0) {
        if (test->settings->domain == AF_INET)
            family = AF_INET;
        else if (test->settings->domain == AF_INET6)
            family = AF_INET6;
        else
            family = AF_INET; /* default to v4 to avoid dual-stack bind issues */
    }

    if (family == AF_INET)
        QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_INET);
    else
        QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_INET6);

    uint16_t listen_port = (test->quic_port > 0) ? (uint16_t)test->quic_port : (uint16_t)test->server_port;
    QuicAddrSetPort(&addr, listen_port);

    if (QUIC_FAILED(status = ctx->api->ListenerStart(ctx->listener, &kQuicAlpn, 1, &addr))) {
        iperf_err(test, "MsQuic ListenerStart failed, 0x%x", status);
        i_errno = IELISTEN;
        return -1;
    }
    if (test->debug)
        iperf_printf(test, "QUIC: listener started on UDP port %u\n", (unsigned)listen_port);

    if (ctx->notify_fds[0] >= 0)
        test->prot_listener = ctx->notify_fds[0];
    return test->prot_listener;
}

int
iperf_quic_accept(struct iperf_test *test)
{
    struct iperf_quic_context *ctx = test->quic_ctx;
    if (!ctx) {
        i_errno = IEACCEPT;
        return -1;
    }

    if (ctx->notify_fds[0] >= 0) {
        uint8_t buf;
        (void)read(ctx->notify_fds[0], &buf, 1);
    }

    pthread_mutex_lock(&ctx->lock);
    struct iperf_quic_stream_ctx *sctx = quic_accept_dequeue(ctx);
    if (!sctx) {
        pthread_mutex_unlock(&ctx->lock);
        i_errno = IEACCEPT;
        return -1;
    }
    int id = ++ctx->next_stream_id;
    quic_map_insert(ctx, id, sctx);
    pthread_mutex_unlock(&ctx->lock);
    return id;
}

int
iperf_quic_connect(struct iperf_test *test)
{
    if (quic_ctx_init(test, 0) < 0) {
        if (i_errno == 0)
            i_errno = IECONNECT;
        return -1;
    }
    struct iperf_quic_context *ctx = test->quic_ctx;
    QUIC_STATUS status;

    if (!ctx->connection) {
        if (QUIC_FAILED(status = ctx->api->ConnectionOpen(ctx->registration, quic_connection_callback, test, &ctx->connection))) {
            i_errno = IECONNECT;
            return -1;
        }
        QUIC_ADDRESS_FAMILY family = QUIC_ADDRESS_FAMILY_UNSPEC;
        if (test->settings->domain == AF_INET)
            family = QUIC_ADDRESS_FAMILY_INET;
        else if (test->settings->domain == AF_INET6)
            family = QUIC_ADDRESS_FAMILY_INET6;

        uint16_t connect_port = (test->quic_port > 0) ? (uint16_t)test->quic_port : (uint16_t)test->server_port;
        if (QUIC_FAILED(status = ctx->api->ConnectionStart(ctx->connection, ctx->configuration, family, test->server_hostname, connect_port))) {
            i_errno = IECONNECT;
            return -1;
        }
        if (test->debug)
            iperf_printf(test, "QUIC: connecting to %s:%u\n", test->server_hostname, (unsigned)connect_port);
    }

    pthread_mutex_lock(&ctx->lock);
    if (!ctx->ready) {
        if (test->settings->connect_timeout > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += test->settings->connect_timeout / 1000;
            ts.tv_nsec += (test->settings->connect_timeout % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000;
            }
            while (!ctx->ready) {
                if (pthread_cond_timedwait(&ctx->cond, &ctx->lock, &ts) == ETIMEDOUT)
                    break;
            }
        } else {
            while (!ctx->ready) {
                pthread_cond_wait(&ctx->cond, &ctx->lock);
            }
        }
    }
    pthread_mutex_unlock(&ctx->lock);

    if (!ctx->ready) {
        i_errno = IECONNECT;
        return -1;
    }

    HQUIC stream = NULL;
    if (QUIC_FAILED(status = ctx->api->StreamOpen(ctx->connection, QUIC_STREAM_OPEN_FLAG_NONE, quic_stream_callback, NULL, &stream))) {
        i_errno = IECONNECT;
        return -1;
    }
    struct iperf_quic_stream_ctx *sctx = quic_stream_ctx_create(stream, test);
    if (!sctx) {
        ctx->api->StreamClose(stream);
        i_errno = IECONNECT;
        return -1;
    }
    ctx->api->SetCallbackHandler(stream, (void *)quic_stream_callback, sctx);
    ctx->api->StreamReceiveSetEnabled(stream, TRUE);
    if (QUIC_FAILED(status = ctx->api->StreamStart(stream, QUIC_STREAM_START_FLAG_IMMEDIATE))) {
        quic_close_stream_handle(ctx, sctx);
        quic_stream_ctx_free(sctx);
        i_errno = IECONNECT;
        return -1;
    }
    if (test->debug)
        iperf_printf(test, "QUIC: client stream started\n");

    pthread_mutex_lock(&ctx->lock);
    int id = ++ctx->next_stream_id;
    quic_map_insert(ctx, id, sctx);
    pthread_mutex_unlock(&ctx->lock);
    return id;
}

int
iperf_quic_attach_stream(struct iperf_test *test, struct iperf_stream *sp, int stream_id)
{
    struct iperf_quic_context *ctx = test->quic_ctx;
    if (!ctx)
        return -1;
    pthread_mutex_lock(&ctx->lock);
    struct iperf_quic_stream_ctx *sctx = quic_map_take(ctx, stream_id);
    pthread_mutex_unlock(&ctx->lock);
    if (!sctx)
        return -1;
    sp->data = sctx;
    return 0;
}

int
iperf_quic_send(struct iperf_stream *sp)
{
    struct iperf_quic_stream_ctx *sctx = (struct iperf_quic_stream_ctx *)sp->data;
    struct iperf_quic_context *ctx = sp->test->quic_ctx;
    if (!sctx || !ctx)
        return NET_HARDERROR;

    if (!sp->pending_size)
        sp->pending_size = sp->settings->blksize;
    size_t size = (size_t)sp->pending_size;
    uint8_t *buf = malloc(size);
    if (!buf)
        return NET_HARDERROR;
    memcpy(buf, sp->buffer, size);

    QUIC_BUFFER qb;
    qb.Length = (uint32_t)size;
    qb.Buffer = buf;
    QUIC_STATUS status = ctx->api->StreamSend(sctx->stream, &qb, 1, QUIC_SEND_FLAG_NONE, buf);
    if (QUIC_FAILED(status)) {
        if (sp->test->debug)
            iperf_printf(sp->test, "QUIC: StreamSend failed, 0x%x\n", status);
        free(buf);
        return NET_HARDERROR;
    }
    sp->pending_size = 0;
    sp->result->bytes_sent += size;
    sp->result->bytes_sent_this_interval += size;
    return (int)size;
}

int
iperf_quic_recv(struct iperf_stream *sp)
{
    struct iperf_quic_stream_ctx *sctx = (struct iperf_quic_stream_ctx *)sp->data;
    if (!sctx)
        return NET_HARDERROR;

    pthread_mutex_lock(&sctx->lock);
    while (sctx->queued_len == 0 && !sctx->recv_shutdown && !sp->done && !sp->test->done) {
        pthread_cond_wait(&sctx->cond, &sctx->lock);
    }
    if (sctx->queued_len == 0) {
        pthread_mutex_unlock(&sctx->lock);
        return 0;
    }

    size_t to_copy = sp->settings->blksize;
    if (to_copy > sctx->queued_len)
        to_copy = sctx->queued_len;
    size_t copied = 0;
    while (copied < to_copy && sctx->head) {
        struct quic_recv_chunk *c = sctx->head;
        size_t avail = c->len - c->off;
        size_t take = to_copy - copied;
        if (take > avail)
            take = avail;
        memcpy(sp->buffer + copied, c->data + c->off, take);
        c->off += take;
        copied += take;
        sctx->queued_len -= take;
        if (c->off == c->len) {
            sctx->head = c->next;
            if (!sctx->head)
                sctx->tail = NULL;
            free(c->data);
            free(c);
        }
    }
    pthread_mutex_unlock(&sctx->lock);
    if (sp->test->state == TEST_RUNNING) {
        sp->result->bytes_received += copied;
        sp->result->bytes_received_this_interval += copied;
    }
    return (int)copied;
}

void
iperf_quic_stream_cleanup(struct iperf_stream *sp)
{
    struct iperf_quic_stream_ctx *sctx = (struct iperf_quic_stream_ctx *)sp->data;
    struct iperf_quic_context *ctx = sp->test->quic_ctx;
    if (!sctx)
        return;
    if (ctx)
        quic_close_stream_handle(ctx, sctx);
    quic_stream_ctx_free(sctx);
    sp->data = NULL;
}

void
iperf_quic_close_listener(struct iperf_test *test)
{
    struct iperf_quic_context *ctx = test->quic_ctx;
    if (!ctx)
        return;
    if (ctx->listener) {
        ctx->api->ListenerStop(ctx->listener);
        ctx->api->ListenerClose(ctx->listener);
        ctx->listener = NULL;
    }
    if (test->prot_listener > -1) {
        FD_CLR(test->prot_listener, &test->read_set);
        FD_CLR(test->prot_listener, &test->write_set);
    }
    if (ctx->notify_fds[0] >= 0) {
        close(ctx->notify_fds[0]);
        ctx->notify_fds[0] = -1;
    }
    if (ctx->notify_fds[1] >= 0) {
        close(ctx->notify_fds[1]);
        ctx->notify_fds[1] = -1;
    }
    test->prot_listener = -1;
}

void
iperf_quic_reset_test(struct iperf_test *test)
{
    iperf_quic_free_test(test);
}

void
iperf_quic_free_test(struct iperf_test *test)
{
    struct iperf_quic_context *ctx = test->quic_ctx;
    if (!ctx)
        return;

    if (ctx->listener) {
        ctx->api->ListenerStop(ctx->listener);
        ctx->api->ListenerClose(ctx->listener);
        ctx->listener = NULL;
    }

    if (ctx->connection) {
        ctx->api->ConnectionShutdown(ctx->connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
        ctx->api->ConnectionClose(ctx->connection);
        ctx->connection = NULL;
    }

    struct quic_stream_map *m = ctx->map;
    while (m) {
        struct quic_stream_map *next = m->next;
        quic_close_stream_handle(ctx, m->ctx);
        quic_stream_ctx_free(m->ctx);
        free(m);
        m = next;
    }
    ctx->map = NULL;

    struct quic_accept_node *a = ctx->accept_head;
    while (a) {
        struct quic_accept_node *next = a->next;
        quic_close_stream_handle(ctx, a->ctx);
        quic_stream_ctx_free(a->ctx);
        free(a);
        a = next;
    }
    ctx->accept_head = ctx->accept_tail = NULL;

    if (ctx->notify_fds[0] >= 0)
        close(ctx->notify_fds[0]);
    if (ctx->notify_fds[1] >= 0)
        close(ctx->notify_fds[1]);

    if (ctx->configuration) {
        ctx->api->ConfigurationClose(ctx->configuration);
        ctx->configuration = NULL;
    }
    if (ctx->registration) {
        ctx->api->RegistrationClose(ctx->registration);
        ctx->registration = NULL;
    }
    if (ctx->api) {
        MsQuicClose(ctx->api);
    }

    pthread_mutex_destroy(&ctx->lock);
    pthread_cond_destroy(&ctx->cond);
    free(ctx);
    test->quic_ctx = NULL;
    test->prot_listener = -1;
}

int
iperf_quic_shutdown_complete(struct iperf_test *test)
{
    if (!test || !test->quic_ctx)
        return 0;
    return test->quic_ctx->shutdown_complete != 0;
}

#else /* HAVE_MSQUIC */

int iperf_quic_init(struct iperf_test *test) { (void)test; i_errno = IEUNIMP; return -1; }
int iperf_quic_listen(struct iperf_test *test) { (void)test; i_errno = IEUNIMP; return -1; }
int iperf_quic_accept(struct iperf_test *test) { (void)test; i_errno = IEUNIMP; return -1; }
int iperf_quic_connect(struct iperf_test *test) { (void)test; i_errno = IEUNIMP; return -1; }
int iperf_quic_send(struct iperf_stream *sp) { (void)sp; i_errno = IEUNIMP; return -1; }
int iperf_quic_recv(struct iperf_stream *sp) { (void)sp; i_errno = IEUNIMP; return -1; }
int iperf_quic_attach_stream(struct iperf_test *test, struct iperf_stream *sp, int stream_id)
{
    (void)test;
    (void)sp;
    (void)stream_id;
    i_errno = IEUNIMP;
    return -1;
}
void iperf_quic_stream_cleanup(struct iperf_stream *sp) { (void)sp; }
void iperf_quic_close_listener(struct iperf_test *test) { (void)test; }
void iperf_quic_reset_test(struct iperf_test *test) { (void)test; }
void iperf_quic_free_test(struct iperf_test *test) { (void)test; }
int iperf_quic_shutdown_complete(struct iperf_test *test) { (void)test; return 0; }

#endif /* HAVE_MSQUIC */
