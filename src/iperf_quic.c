/*
 * iperf, Copyright (c) 2014-2022, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>
#include <ctype.h>

#include <openssl/bio.h>
#include <openssl/err.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_udp.h"
#include "iperf_quic.h"
#include "timer.h"
#include "net.h"
#include "cjson.h"

#ifdef HAVE_QUIC_NGTCP2

const char* quic_key_pem = 
"-----BEGIN RSA PRIVATE KEY-----\n"
"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\n"
"Fi02nu1fk6yAECh9srGIoqqPcTPCDg==\n"
"-----END RSA PRIVATE KEY-----\n";

// 2. Define the Certificate as a constant string (PEM format)
const char* quic_cert_pem = 
"-----BEGIN CERTIFICATE-----\n"
"MIIFCTCCAvGgAwIBAgIUZfjf9f6g5VuVmebED0VcNDC4i5wwDQYJKoZIhvcNAQEL\n"
"BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI1MTEwMzE5MjY0OFoXDTI2MTEw\n"
"MzE5MjY0OFowFDESMBAGA1UEAwwJbG9jYWxob3N0MIICIjANBgkqhkiG9w0BAQEF\n"
"AAOCAg8AMIICCgKCAgEAyOW1Y7Tq4rIlFhu95otqodBUdNKNeqqp2F+wBmALgiQA\n"
"O3dPtpBH2vNncsctUH/8oDgE3F4vj8ivujstSvZJnGw5mNNsKtMw1UnZdDDaz1zg\n"
"Av8HlK6Kq3w7NQJ7zaPKC1bWEKWquJ0DXKdpB5ic+dmmPdYyxb1f9SoTE6Ux+Me3\n"
"0rZE3Zwkm8yLoyZ3OuZd9zJBPYGW2IwJtfODIp1GxTIcx8hkDxQGeCRwxjbk2gAT\n"
"K85kw+/fPW3oRmlV9svChRYXyjlagZIf9XRwIpl30QCCfrJ8rjfQbDdMydvRgck0\n"
"016opemYDUJdU7DRfXmE3d0bmon/ouzq8IBR95YaFfRYOINrYz0K26+t1PM5jDsn\n"
"v8mdIcHAkIsAg/0Nqmcm2GdCaa2ioiy4R3cNZhJAvTg2ZVv1Fzx66QbULVVkOcrH\n"
"9s9wStBjn7KHL1MPLO9A9G4maUWHXKt2l/zfXgOc08Zmd/qZmlHNgZS7TXQDnyWF\n"
"PHGApnIygCf3k5k4fY5+As/mi1O7w8NNBYnJBdjeeJ+JQnWoDlOX27CLsCikmEH4\n"
"B5P4R0VRZHbWPqcENmp9fAEgVRBg1eZwIZ+iijjJPZau/ECPPeNsBySX4DIvDQwA\n"
"9W6s5I5iGoX2ND28/adFJlPwaX8LxAjQ0aHZR4VxxtgU0Mlns/lekJes7ZbcBmEC\n"
"AwEAAaNTMFEwHQYDVR0OBBYEFBI7OSF0nAKm/huBOp8EBaKq5iqWMB8GA1UdIwQY\n"
"MBaAFBI7OSF0nAKm/huBOp8EBaKq5iqWMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZI\n"
"hvcNAQELBQADggIBACynBaf508cJKTjLv1u0GnL/zWn9kAiaK87fcwbVWy5Rh3ZO\n"
"UsThs701X4DkSTQzlsl5MDnFx4u3F4ZO/t09nWZpIluQRkfBvqYZR2JIqbuuaw+1\n"
"42z9LF0iaQQnsCV9zKh0NESrIOZ4YsdJQpG7a436Nx083HuNsqykoD8/hIiTCAqB\n"
"XAF39QE67dpQJijOgG2EW7Git27703KIrpmvIDn/OQG1kmi6wfNIaLv8x/JCORaZ\n"
"OfysH0bhsbarneyvq0jAXoQGG1jT0OD4yZLIESMBHvVc3UFapFTXVvDS3h6J0dke\n"
"3cISEtKAztk1ryjQQGBpWT8X/HQZOdL5Ld5c0N1XKPWPgIIUEWaejedwoWr6ikh+\n"
"UPbTD5dGt9T9vA9Ijwil4EOjSHlpxghd/8wq9j1P3fthQTFtii0YK6NHh0S30+Xx\n"
"wQ44XqsGd6yShJEOSZwSfTI+RIifN2bPNI2byET6kU82jrQL5A3GVYysfokEYbx+\n"
"IFkS+tNW+r+aW0NF8N857P5tu60rghoNX/CW2QSp1su7ZIX8u5IHvk5UrD5s2pRJ\n"
"kHadMSD9NcMzHDu+MTaslH6tzXV3RdCmHseh9xPUiYrrQEkPLvFQITRCUm1fOJDl\n"
"Mj896a+OWRPjH4IZQHDO3Js5weJMxLLKi5ECtflwrX1ocmFgkAtKx5KZLkmX\n"
"-----END CERTIFICATE-----\n";


static int quic_flush_tx(struct iperf_quic_conn_data *quic_conn_data);
static ngtcp2_tstamp quic_check_and_handle_expiry(struct iperf_quic_conn_data *quic_conn_data);

// ============ >>>>> Standard for iperf3 ====================

/* iperf_quic_recv
 *
 * receives the data for QUIC
 */
int
iperf_quic_recv(struct iperf_stream *sp)
{
    int r, rv;
    struct iperf_test *test = sp->test;
    struct iperf_quic_conn_data *quic_conn_data = &sp->quic_conn_data;
    ngtcp2_tstamp ts;
    uint64_t rcv_timeout_ms;
    uint8_t buf[65536];

    rcv_timeout_ms = (test->settings->rcv_timeout.secs * SEC_TO_mS) + (test->settings->rcv_timeout.usecs / mS_TO_US);
    r = Nread_something_with_timeout(sp->socket, (char *)buf, sizeof(buf), Pquic, rcv_timeout_ms);

    if (r < 0) {
        i_errno = IEQUICRECV;
        return r;
    }

    ts = iperf_time_now_in_ns();
    rv = ngtcp2_conn_read_pkt(quic_conn_data->pconn, &quic_conn_data->path.path, NULL, buf, (size_t)r, ts);

    if (rv < 0) {
        if (test->debug_level >= DEBUG_LEVEL_INFO)
            printf("iperf_quic_recv: ngtcp2_conn_read_pkt for socket=%d, stream_id=%ld: error %s\n", sp->socket, quic_conn_data->quic_stream_id, ngtcp2_strerror(rv));
        i_errno = IEQUICRECV;
        return rv;
    }

    (void)quic_flush_tx(quic_conn_data);

    return 0;
}


/* iperf_quic_send
 *
 * sends the data for QUIC
 */

int
iperf_quic_send(struct iperf_stream *sp)
{
    struct iperf_test *test = sp->test;
    int r;
    int size = sp->settings->blksize;
    struct iperf_quic_conn_data *quic_conn_data = &sp->quic_conn_data;
    ngtcp2_tstamp ts;
    ngtcp2_ssize datalen;
    uint8_t buf[65536];
    ngtcp2_ssize sent = 0;
    ngtcp2_ssize nwrite;
    ngtcp2_pkt_info pi = {0};
    ngtcp2_tstamp timeout;
    int nread;

    // Sending the full blocksize message
    while (sent < size) {
        struct pollfd pfd = { .fd = sp->socket, .events = POLLIN };
        int pr = poll(&pfd, 1, 0); // Non-blocking poll
        if (pr < 0) {
            iperf_err(test, "iperf_quic_send: poll failed: errno=%d-%s", errno, strerror(errno));
            i_errno = IEQUICSEND;
            return -i_errno;
        }

        if (pr == 0) {
            timeout = quic_check_and_handle_expiry(quic_conn_data);
            if (timeout == UINT64_MAX) {
                iperf_err(test, "iperf_quic_send: quic_check_and_handle_expiry failed for socket=%d", sp->socket);
                return -i_errno;
            }
        } else if (pfd.revents & POLLIN) {
            nread = recv(sp->socket, buf, sizeof(buf), 0);
            if (nread < 0) {
                iperf_err(test, "iperf_quic_send: recv failed on socket=%d: errno=%d-%s", sp->socket, errno, strerror(errno));
                i_errno = IEQUICSEND;
                return -i_errno;
            } else if (nread > 0) {
                (void)ngtcp2_conn_read_pkt(quic_conn_data->pconn, &quic_conn_data->path.path, NULL, buf, (size_t)nread, iperf_time_now_in_ns());
            } else if (nread == 0) {
                timeout = quic_check_and_handle_expiry(quic_conn_data);
                if (timeout == UINT64_MAX) {
                    iperf_err(test, "iperf_quic_send: after read, quic_check_and_handle_expiry failed for socket=%d", sp->socket);
                    return -i_errno;
                }
            }
        }

        datalen = -1;
        ts = iperf_time_now_in_ns();

        do { // Filling the QUIC buffer to be sent
            nwrite = ngtcp2_conn_write_stream(
                quic_conn_data->pconn, &quic_conn_data->path.path, &pi,
                (uint8_t *)buf, sizeof(buf),
                &datalen, NGTCP2_WRITE_STREAM_FLAG_MORE, quic_conn_data->quic_stream_id, (uint8_t *)sp->buffer + sent, size - sent,
                ts);

            if (test->debug_level >= DEBUG_LEVEL_DEBUG && datalen >= 0)
                printf("iperf_quic_send: AFTER 1st-type ngtcp2_conn_write_stream returned %zd-%s, blksize=%d, sent=%ld, datalen=%zd\n", nwrite, ngtcp2_strerror(nwrite), size, sent, datalen);

            if (datalen > 0) sent += datalen;

        } while (nwrite == NGTCP2_ERR_WRITE_MORE && sent < size);

        if (nwrite == NGTCP2_ERR_WRITE_MORE) {
            datalen = -1;
            nwrite = ngtcp2_conn_write_stream(
                quic_conn_data->pconn, &quic_conn_data->path.path, &pi,
                (uint8_t *)buf, sizeof(buf),
                &datalen, NGTCP2_WRITE_STREAM_FLAG_MORE, quic_conn_data->quic_stream_id, (uint8_t *)NULL, 0,
                ts);

            if (test->debug_level >= DEBUG_LEVEL_DEBUG && datalen >= 0)
                printf("iperf_quic_send: AFTER 2nd-type ngtcp2_conn_write_stream returned %zd-%s, blksize=%d, sent=%ld, datalen=%zd\n", nwrite, ngtcp2_strerror(nwrite), size, sent, datalen);
        }

        if (nwrite < 0) {
            if (r == NET_SOFTERROR && sp->test->debug_level >= DEBUG_LEVEL_INFO)
                printf("iperf_quic_send: ngtcp2_conn_write_stream error on socket=%d, stream_id=%ld: %s\n", sp->socket, quic_conn_data->quic_stream_id, ngtcp2_strerror(nwrite));
            i_errno = IEQUICSEND;
            return (int)nwrite; // Error
        }

        if (nwrite > 0) { // Sending the QUIC buffer
            r = Nwrite(sp->socket, (char *)buf, nwrite, Pquic);
            if (test->debug_level >= DEBUG_LEVEL_DEBUG)
                printf("iperf_quic_send: AFTER Nwrite returned %d, to be sent=%ld\n", r, sent);

            if (r < 0) {
                if (r == NET_SOFTERROR && sp->test->debug_level >= DEBUG_LEVEL_INFO)
                    printf("QUIC send failed on NET_SOFTERROR. errno=%s\n", strerror(errno));
                i_errno = IEQUICSEND;
                return r;
            }
        }

        if (test->debug_level >= DEBUG_LEVEL_DEBUG)
            printf("iperf_quic_send: WHILE iteration end blksize=%d, sent=%ld, datalen=%zd\n", size, sent, datalen);

        ts = iperf_time_now_in_ns();
        ngtcp2_conn_update_pkt_tx_time(quic_conn_data->pconn, ts);

    } // while() to send full blocksize message

    ++sp->packet_count;

    sp->result->bytes_sent += sent;
    sp->result->bytes_sent_this_interval += sent;

    if (sp->test->debug_level >=  DEBUG_LEVEL_DEBUG)
        printf("QUIC sent %d bytes of %d, total %" PRIu64 "\n", r, sp->settings->blksize, sp->result->bytes_sent);

    return sent;
}

// ============ <<<<<<<< Standard for iperf3 ====================


// ============== QUIC SPECIFIC FUNCTIONS (CBs, etc.) ==============

static void quic_ngtcp2_log_printf_cb(void *quic_conn_data, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    printf("[ngtcp2 log_printf]  %.8lu[us]:",  iperf_time_now_in_usecs()%100000000);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static void quic_rand_cb(unsigned char *dest, size_t destlen, const ngtcp2_rand_ctx *ctx)
{
  int rv;

  rv = readentropy(dest, destlen);
  if(rv != 0) {
    memset(dest, 0, destlen); /* if readentropy failed, just fill 0 and call it *random*. */
  }
}

/***** >>>>>>> removing quic_acked_stream_data_offset_cb function as currently not used ***********
// Callback to identify acked data
static int quic_acked_stream_data_offset_cb(ngtcp2_conn *conn, int64_t stream_id,
                             uint64_t offset, uint64_t datalen, void *user_data,
                             void *stream_user_data) {
    struct iperf_quic_conn_data *quic_conn_data = user_data;

    printf("[CB] quic_acked_stream_data_offset_cb: called socket=%d, stream_id=%ld, offset=%ld, datalen=%ld\n", quic_conn_data->sp->socket, quic_conn_data->quic_stream_id, offset, datalen);

    return 0;
}
***** <<<<<<<<< removing quic_acked_stream_data_offset_cb function as currently not used ***********/

// Callback to provide new connection IDs
static int quic_get_new_connection_id_cb(ngtcp2_conn *conn, ngtcp2_cid *cid, uint8_t *token, size_t cidlen, void *quic_conn_data) {
    int i;
    struct iperf_test *test = ((struct iperf_quic_conn_data *)quic_conn_data)->sp->test;

    // Generate a new connection ID (cid) and token
    // For simplicity, we use random bytes here. In production, ensure uniqueness.
    if (cidlen > sizeof(cid->data)) {
        iperf_err(test, "[CB] quic_get_new_connection_id_cb: cidlen=%zu should be up to %zu called, socket=%d", cidlen, sizeof(cid->data), ((struct iperf_quic_conn_data *)quic_conn_data)->sp->socket);
        i_errno = IEQUICCONNECTIONID;
        return -1;
    }
    if (readentropy(cid->data, cidlen) != 0) {
        iperf_err(test, "[CB] quic_get_new_connection_id_cb: readentropy for cid failed, socket=%d", ((struct iperf_quic_conn_data *)quic_conn_data)->sp->socket);
        i_errno = IEQUICCONNECTIONID;
        return -1;
    }
    cid->datalen = cidlen;

    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        printf("quic_get_new_connection_id_cb: cid=");
        for (i = 0; i < cid->datalen; i++) {
            printf("%02x", cid->data[i]);
        }
        printf("\n");
    }

    // For token, we can use a simple scheme; here we just fill with random bytes
    if (readentropy(token, NGTCP2_STATELESS_RESET_TOKENLEN) != 0) {
        iperf_err(test, "[CB] quic_get_new_connection_id_cb: readentropy for token failed, socket=%d", ((struct iperf_quic_conn_data *)quic_conn_data)->sp->socket);
        i_errno = IEQUICCONNECTIONID;
        return -1;
    }

    return 0;
}

static int quic_handshake_completed_cb(ngtcp2_conn *conn, void *user_data) {
    if (((struct iperf_quic_conn_data *)user_data)->sp->test->debug_level >= DEBUG_LEVEL_INFO)
        printf("[CB] quic_handshake_completed_cb: called for socket=%d\n", ((struct iperf_quic_conn_data *)user_data)->sp->socket);

    // This signifies the application can start sending/receiving application data.
    ((struct iperf_quic_conn_data *)user_data)->quic_handshake_completed = 1;
    return 0;
}

static int quic_stream_open_cb(ngtcp2_conn *conn, int64_t stream_id, void *user_data) {
  if (((struct iperf_quic_conn_data *)user_data)->sp->test->debug_level >= DEBUG_LEVEL_INFO)
    printf("[CB] quic_stream_open_cb: called for socket=%d, stream_id=%ld\n", ((struct iperf_quic_conn_data *)user_data)->sp->socket, stream_id);

  ((struct iperf_quic_conn_data *)user_data)->quic_stream_id = stream_id;
  return 0;
}


// ---------- ALPN selection (server) ----------
static int quic_alpn_select_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                          const unsigned char *in, unsigned int inlen, void *arg) {
  (void)ssl;
  (void)arg;

  const unsigned char *p = in;
  unsigned int left = inlen;

  while (left > 0) {
    unsigned int n = p[0];
    p++;
    left--;
    if (n > left) break;

    if (n == strlen(IPERF_ALPN_STR) && memcmp(p, IPERF_ALPN_STR, n) == 0) {
      *out = p;
      *outlen = (unsigned char)n;
      
      //printf("[CB] server: selected ALPN: %.*s\n", n, p);
      return SSL_TLSEXT_ERR_OK;
    }

    p += n;
    left -= n;
  }

  //printf("[quic_alpn_select_cb] server: none ALPN matched.\n");
  return SSL_TLSEXT_ERR_NOACK;
}


static int quic_recv_stream_data_cb(ngtcp2_conn *conn, uint32_t flags, int64_t stream_id,
                                      uint64_t offset, const uint8_t *data, size_t datalen,
                                      void *quic_conn_data_in, /*const ngtcp2_pkt_info *pi*/ void *stream_user_data)
{
    struct iperf_quic_conn_data *quic_conn_data = (struct iperf_quic_conn_data *)quic_conn_data_in;
    struct iperf_stream *sp = quic_conn_data->sp;

    // A stream data frame was received.
    if (sp->test->debug_level >= DEBUG_LEVEL_DEBUG)
        printf("[CB]: Received stream data on socket %d, stream %ld\n", sp->socket, stream_id);

    quic_conn_data->got_app_data = 1;

    /* Only count bytes received while we're in the correct state. */
    if (sp->test->state == TEST_RUNNING) {
	      sp->result->bytes_received += datalen;
	      sp->result->bytes_received_this_interval += datalen;
    }
    else {
	      if (sp->test->debug_level >= DEBUG_LEVEL_INFO)
	          printf("Late receive, state = %d-%s\n", sp->test->state, state_to_text(sp->test->state));
    }

    return 0; // Return 0 on success
}


static char algo_str_reno[] = "reno";
static char algo_str_cubic[] = "cubic";
static char algo_str_bbr[] = "bbr";
static ngtcp2_cc_algo quic_init_cc_algo(struct iperf_test *test, ngtcp2_cc_algo default_cc_algo) {
    ngtcp2_cc_algo cc_algo = default_cc_algo;
    char *pcalgo, *p;

    if (test->congestion) {
        pcalgo = strdup(test->congestion);
        for (p = pcalgo; *p; ++p) {
            *p = tolower((unsigned char)*p);
        }

        if (strcmp(pcalgo, algo_str_reno) == 0) {
            cc_algo = NGTCP2_CC_ALGO_RENO;
        } else if (strcmp(pcalgo, algo_str_cubic) == 0) {
            cc_algo = NGTCP2_CC_ALGO_CUBIC;
        } else if (strcmp(pcalgo, algo_str_bbr) == 0) {
            cc_algo = NGTCP2_CC_ALGO_BBR;
        } else {
            iperf_err(test, "Unknown QUIC congestion control algorithm: %s;  Using default", test->congestion);
        }

        free(pcalgo);
    }

    if (test->congestion_used) {
	    free(test->congestion_used);
    }

    switch (cc_algo) {
        case NGTCP2_CC_ALGO_RENO:
            test->congestion_used = strdup(algo_str_reno);
            break;
        case NGTCP2_CC_ALGO_CUBIC:
            test->congestion_used = strdup(algo_str_cubic);
            break;
        case NGTCP2_CC_ALGO_BBR:
            test->congestion_used = strdup(algo_str_bbr);
            break;
        default:
            iperf_err(test, "Unexpected unknown QUIC congestion algorithm; Using CUBIC");
            cc_algo = NGTCP2_CC_ALGO_CUBIC;
            test->congestion_used = strdup(algo_str_cubic);
            break;
    }

    if (test->debug_level >= DEBUG_LEVEL_INFO)
        printf("QUIC Congestion algorithm is %s\n", test->congestion_used);

    return cc_algo;
}

/* Init ngtcp2 params */
static void quic_init_params(struct iperf_quic_conn_data *quic_conn_data, ngtcp2_transport_params *params) {
    struct iperf_test *test = quic_conn_data->sp->test;

    memset(params, 0, sizeof(ngtcp2_transport_params));
    ngtcp2_transport_params_default(params);

    params->initial_max_data = NGTCP2_MAX_VARINT; //TBD: needed?  Another value?
    params->initial_max_stream_data_uni = NGTCP2_MAX_VARINT; //TBD: what should be the value for this and the following? IPERF_QUIC_DEFAULT_MAX_RECV_UDP_PAYLOAD_SIZE
    params->initial_max_stream_data_bidi_local = NGTCP2_MAX_VARINT;
    params->initial_max_stream_data_bidi_remote = NGTCP2_MAX_VARINT;

    params->initial_max_streams_bidi = 1;
    params->initial_max_streams_uni = 1;
    params->max_idle_timeout = test->settings->snd_timeout; // in milliseconds
    params->max_udp_payload_size = IPERF_QUIC_DEFAULT_MAX_RECV_UDP_PAYLOAD_SIZE;

    if (quic_conn_data->sp->test->debug_level >= DEBUG_LEVEL_INFO)
        printf("quic_init_params: Initialized QUIC transport parameters\n");
}

/* Init ngtcp2 settings */
static void quic_init_settings(struct iperf_quic_conn_data *quic_conn_data, ngtcp2_settings *settings) {
    struct iperf_stream *sp = quic_conn_data->sp;
    struct iperf_test *test = sp->test;
    int max_tx_udp_payload_size;

    memset(settings, 0, sizeof(ngtcp2_settings));
    ngtcp2_settings_default(settings);
    settings->cc_algo = quic_init_cc_algo(test, settings->cc_algo);
    settings->initial_ts = iperf_time_now_in_ns(); // Current time for RTT calculation
    settings->no_tx_udp_payload_size_shaping = 1;
    //settings->max_stream_window = IPERF_QUIC_MAX_STREAM_WINDOWS; //TBD: needed? Value OK?
    max_tx_udp_payload_size = test->settings->blksize + (2 * 1024); // Some overhead
    if (max_tx_udp_payload_size > IPERF_QUIC_MAX_TX_PAYLOAD_SIZE) max_tx_udp_payload_size = IPERF_QUIC_MAX_TX_PAYLOAD_SIZE;
    settings->max_tx_udp_payload_size = max_tx_udp_payload_size;

    if (test->debug_level >= DEBUG_LEVEL_DEBUG)
        settings->log_printf = quic_ngtcp2_log_printf_cb; // Enable logging to see what happens

    if (quic_conn_data->sp->test->debug_level >= DEBUG_LEVEL_INFO)
        printf("quic_init_settings: Initialized QUIC settings\n");
}

/* Init callbacks */
static void quic_init_callbacks(struct iperf_quic_conn_data *quic_conn_data, ngtcp2_callbacks *callbacks) {
    memset(callbacks, 0, sizeof(ngtcp2_callbacks));
    callbacks->get_new_connection_id = quic_get_new_connection_id_cb; // Custom or default
    callbacks->rand = quic_rand_cb;
    callbacks->handshake_completed = quic_handshake_completed_cb;
    callbacks->stream_open = quic_stream_open_cb;
    //callbacks->acked_stream_data_offset = quic_acked_stream_data_offset_cb;

    callbacks->client_initial = ngtcp2_crypto_client_initial_cb;

    callbacks->recv_client_initial = ngtcp2_crypto_recv_client_initial_cb;
    callbacks->recv_stream_data = quic_recv_stream_data_cb;

    callbacks->recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb;
    callbacks->encrypt = ngtcp2_crypto_encrypt_cb;
    callbacks->decrypt = ngtcp2_crypto_decrypt_cb;
    callbacks->hp_mask = ngtcp2_crypto_hp_mask_cb;
    callbacks->recv_retry = ngtcp2_crypto_recv_retry_cb;
    callbacks->update_key = ngtcp2_crypto_update_key_cb;
    callbacks->delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb;
    callbacks->delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb;
    callbacks->get_path_challenge_data = ngtcp2_crypto_get_path_challenge_data_cb;
    callbacks->version_negotiation = ngtcp2_crypto_version_negotiation_cb;

    if (quic_conn_data->sp->test->debug_level >= DEBUG_LEVEL_INFO)
        printf("quic_init_callbacks: Initialized QUIC callbacks\n");
}

/*
 * Our callback from the SSL/TLS layers.
 */
static void quic_ossl_trace(int write_p, int ssl_ver, int content_type,
                       const void *buf, size_t buf_len, SSL *ssl,
                       void *arg)
{
  const char *verstr = "???";
  unsigned char *pbuf = (unsigned char *)buf;
  char *content_type_str;
  char *header_type_str;
  char *handshake_type_str;
  char *write_p_str = write_p ? "SENT" : "RECEIVED";

  switch(ssl_ver) {
#ifdef SSL2_VERSION /// removed in recent versions
  case SSL2_VERSION:
    verstr = "SSLv2";
    break;
#endif
#ifdef SSL3_VERSION
  case SSL3_VERSION:
    verstr = "SSLv3";
    break;
#endif
  case TLS1_VERSION:
    verstr = "TLSv1.0";
    break;
#ifdef TLS1_1_VERSION
  case TLS1_1_VERSION:
    verstr = "TLSv1.1";
    break;
#endif
#ifdef TLS1_2_VERSION
  case TLS1_2_VERSION:
    verstr = "TLSv1.2";
    break;
#endif
#ifdef TLS1_3_VERSION  // OpenSSL 1.1.1+, all forks
  case TLS1_3_VERSION:
    verstr = "TLSv1.3";
    break;
#endif
  case 0:
    break;
  default:
    break;
  }

    switch (content_type) {
        case SSL3_RT_CHANGE_CIPHER_SPEC:
            content_type_str = "ChangeCipherSpec";
            break;
        case SSL3_RT_ALERT:
            content_type_str = "Alert";
            break;
        case SSL3_RT_HANDSHAKE:
            content_type_str = "Handshake";
            break;
        case SSL3_RT_APPLICATION_DATA:
            content_type_str = "ApplicationData";
            break;
        case SSL3_RT_HEADER: // Pseudo content type for record header
            content_type_str = "RecordHeader";
            break;
        case SSL3_RT_INNER_CONTENT_TYPE: // Pseudo content type for TLSv1.3 inner content type
            content_type_str = "InnerContentType";
            break;
        default:
            content_type_str = "Unknown";
            break;
    }

    printf("[quic_ossl_trace %s] %.8lu[us]: write_p=%s, ssl_ver=%d-%s, content_type=%d-%s",
         (char *)arg, iperf_time_now_in_usecs()%100000000, write_p_str, ssl_ver, verstr, content_type, content_type_str);
    if (content_type == SSL3_RT_HEADER && buf_len >= 1) { 
        switch (pbuf[0]) {
            case SSL3_RT_CHANGE_CIPHER_SPEC: header_type_str = "ChangeCipherSpec"; break;
            case SSL3_RT_ALERT: header_type_str = "Alert"; break;
            case SSL3_RT_HANDSHAKE: header_type_str = "Handshake"; break;
            case SSL3_RT_APPLICATION_DATA: header_type_str = "ApplicationData"; break;
            default: header_type_str = "OtherHeaderType"; break;
        }
        printf("-%s", header_type_str);
    } else if (content_type == SSL3_RT_ALERT && buf_len >= 2) {
        char *alert_level = (pbuf[0] == 1 ? "warning" : (pbuf[0] == 2 ? "fatal" : "unknown"));
        printf(", level=%s, description=%d", alert_level, pbuf[1]);
    }
    else if (content_type == SSL3_RT_HANDSHAKE && buf_len >= 1) {
        switch (pbuf[0]) {
            case SSL3_MT_CLIENT_HELLO: handshake_type_str = "ClientHello"; break;
            case SSL3_MT_SERVER_HELLO: handshake_type_str = "ServerHello"; break;
            case SSL3_MT_CERTIFICATE: handshake_type_str = "Certificate"; break;
            case SSL3_MT_SERVER_KEY_EXCHANGE: handshake_type_str = "ServerKeyExchange"; break;
            case SSL3_MT_CERTIFICATE_REQUEST: handshake_type_str = "CertificateRequest"; break;
            case SSL3_MT_CERTIFICATE_VERIFY: handshake_type_str = "CertificateVerify"; break;
            case SSL3_MT_CLIENT_KEY_EXCHANGE: handshake_type_str = "ClientKeyExchange"; break;
            case SSL3_MT_FINISHED: handshake_type_str = "Finished"; break;
            default: handshake_type_str = "OtherHandshakeType"; break;
        }
        printf("-%s", handshake_type_str);
    }
    printf(", buf_len=%zu\n", buf_len);
}

static ngtcp2_conn *quic_get_conn(ngtcp2_crypto_conn_ref *quic_conn_ref)
{
  struct iperf_quic_conn_data *quic_conn_data = quic_conn_ref->user_data;

  return quic_conn_data->pconn;
}

// Convert IPv4-mapped IPv6 address to regular IPv4 address string (from iperf_api.c)
static int
mapped_v4_to_regular_v4(char *str)
{
    char *prefix = "::ffff:";
    int prefix_len;

    prefix_len = strlen(prefix);
    if (strncmp(str, prefix, prefix_len) == 0) {
	int str_len = strlen(str);
	memmove(str, str + prefix_len, str_len - prefix_len + 1);
	return 1;
    }
    return 0;
}

// ---------- TX helpers ----------
static int quic_flush_tx(struct iperf_quic_conn_data *quic_conn_data) {
    uint8_t out[2048];

    struct iperf_stream *sp = quic_conn_data->sp;
    struct iperf_test *test = sp->test;
    char role = quic_conn_data->sp->test->role;

    static char quic_msg[] = "QUIC HELLO";

    for(;;) {
        ngtcp2_ssize datalen;
        ngtcp2_tstamp ts;
        ngtcp2_vec vec;
        const ngtcp2_vec *vecp = NULL;
        size_t veccnt = 0;
        int64_t sid = quic_conn_data->quic_stream_id;
        ngtcp2_ssize nwrite, totwrite, ntowrite;
        ssize_t sent;

        if (role == 'c' && !quic_conn_data->quic_hello_writing && quic_conn_data->quic_stream_id >= 0) {
            // Sending hello message from client after stream was established
            if (test->debug_level >= DEBUG_LEVEL_INFO)
                printf("quic_flush_tx: filling HELLO msg into, stream_id=%ld\n", quic_conn_data->quic_stream_id);

            quic_conn_data->quic_hello_writing = 1;
            vec.base = (uint8_t *)(quic_msg);
            vec.len = strlen(quic_msg);
            vecp = &vec;
            veccnt = 1;
        }

        totwrite = 0;
        ntowrite = (veccnt > 0) ? vec.len : 0;
        do { // while
            datalen = -1;
            ts = iperf_time_now_in_ns();
            nwrite = ngtcp2_conn_writev_stream(
                quic_conn_data->pconn, &quic_conn_data->path.path, NULL,
                out, sizeof(out),
                &datalen,
                NGTCP2_WRITE_STREAM_FLAG_NONE, sid, vecp, veccnt,
                ts);

            if (nwrite < 0) {
                iperf_err(sp->test, "quic_flush_tx: ngtcp2_conn_writev_stream error on socket=%d, stream_id=%ld: %s", sp->socket, sid, ngtcp2_strerror(nwrite));
                return -1;
            }

            if (veccnt > 0 && datalen > 0) {
                totwrite += datalen;
                ntowrite -= datalen;
                vec.base = (uint8_t *)(quic_msg + datalen);
                vec.len = strlen(quic_msg) - datalen;
            }

            if (nwrite > 0 ) {
                sent = Nwrite(sp->socket, (const char *)out, nwrite, Pudp);

                if (sent < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
                    iperf_err(sp->test, "quic_flush_tx: Nwrite error on socket=%d, stream_id=%ld: %s", sp->socket, sid, strerror(errno));
                    return -1;
                }
            }

            ngtcp2_conn_update_pkt_tx_time(quic_conn_data->pconn, ts);

        } while (ntowrite > 0);

        if (test->debug_level >= DEBUG_LEVEL_DEBUG)
            printf("quic_flush_tx sent total of %zd bytes, veccnt=%ld\n", totwrite, veccnt);

        return totwrite;

    }

    return 0;
}

static void quic_connection_flush(struct iperf_quic_conn_data *quic_conn_data) {
    uint8_t out[2048];
    ngtcp2_ccerr ccerr;
    ngtcp2_ccerr_default(&ccerr);

    ngtcp2_tstamp ts = iperf_time_now_in_ns();
    ngtcp2_ssize nwrite = ngtcp2_conn_write_connection_close(
        quic_conn_data->pconn, &quic_conn_data->path.path, /*pi=*/NULL,
        out, sizeof(out), &ccerr, ts);

    if (nwrite > 0) {
        (void)Nwrite(quic_conn_data->sp->socket, (const char *)out, nwrite, Pudp);
        ngtcp2_conn_update_pkt_tx_time(quic_conn_data->pconn, ts);
    }

}

/* Check whether quic connection has expired,and handle expiry if so.
 * Returns time remaining [ns] for expiry (including 0) on success, UINT64_MAX on error.
 */
static ngtcp2_tstamp
quic_check_and_handle_expiry(struct iperf_quic_conn_data *quic_conn_data)
{
    int rv;
    ngtcp2_tstamp ts;
    ngtcp2_tstamp expiry = ngtcp2_conn_get_expiry(quic_conn_data->pconn);

    if (expiry == UINT64_MAX) {
        expiry = 0; // No expiry scheduled
    } else {
        ts = iperf_time_now_in_ns();
        if (expiry >= ts) {
            expiry -= ts;
        } else { // expired
            rv = ngtcp2_conn_handle_expiry(quic_conn_data->pconn, ts);
            if (rv < 0) {
                iperf_err(quic_conn_data->sp->test, "quic_check_and_handle_expiry: ngtcp2_conn_handle_expiry error on socket=%d: %s", quic_conn_data->sp->socket, ngtcp2_strerror(rv));
                i_errno = IEQUICEXPIRED;
                expiry = UINT64_MAX;
            } else {
                expiry = 0; // expired now handled
            }
        }
    }

    return expiry;
}


void
iperf_quic_delete_connection(struct iperf_quic_conn_data *quic_conn_data)
{
    struct iperf_stream *sp = quic_conn_data->sp;

    if (sp && sp->test->debug_level >= DEBUG_LEVEL_INFO)
        printf("iperf_quic_delete_connection: closing QUIC connection for socket=%d, stream_id=%" PRIu64 "\n", sp->socket, quic_conn_data->quic_stream_id);
        
     // Close connection
    if (quic_conn_data->pconn) {
        quic_connection_flush(quic_conn_data);
        ngtcp2_conn_del(quic_conn_data->pconn); // Free connection
        quic_conn_data->pconn = NULL;
    }
    if (quic_conn_data->ng_ssl_ctx) {
        ngtcp2_crypto_ossl_ctx_del(quic_conn_data->ng_ssl_ctx); // Free crypto context
        quic_conn_data->ng_ssl_ctx = NULL;
    }
    if (quic_conn_data->ssl) {
        SSL_free(quic_conn_data->ssl); // Free SSL object
        quic_conn_data->ssl = NULL;
    }
    if (quic_conn_data->ssl_ctx) {
        SSL_CTX_free(quic_conn_data->ssl_ctx); // Free SSL context
        quic_conn_data->ssl_ctx = NULL;
    }
    quic_conn_data->sp = NULL;
}



// ---------- QUIC Initial header parse (just enough to get DCID/SCID + version) ----------
static int quic_parse_initial_dcid_scid(struct iperf_stream *sp, const uint8_t *pkt, size_t pktlen,
                                uint32_t *pversion,
                                ngtcp2_cid *dcid, ngtcp2_cid *scid)
{
    struct iperf_test *test = sp->test;

    if (pktlen < 6) {
        iperf_err(test, "quic_parse_initial_dcid_scid: packet length should be at least 6 but is %ld", pktlen);
        i_errno = IEQUICPARSEID;
        return -1;
    }
    if ((pkt[0] & 0x80) == 0) {
        iperf_err(test, "quic_parse_initial_dcid_scid: packet header is not long");
        i_errno = IEQUICPARSEID;
        return -1;  // not a long header
    }

    *pversion = ((uint32_t)pkt[1] << 24) | ((uint32_t)pkt[2] << 16) |
                ((uint32_t)pkt[3] << 8) | (uint32_t)pkt[4];

    size_t pos = 5;
    uint8_t dcidlen = pkt[pos++];

    if (pktlen < pos + dcidlen + 1) {
        iperf_err(test, "quic_parse_initial_dcid_scid: packet length should be at least %ld but is %ld", pos + dcidlen + 1, pktlen);
        i_errno = IEQUICPARSEID;
        return -1;
    }
    ngtcp2_cid_init(dcid, pkt + pos, dcidlen);
    pos += dcidlen;

    uint8_t scidlen = pkt[pos++];
    if (pktlen < pos + scidlen) {
        iperf_err(test, "quic_parse_initial_dcid_scid ERROR: packet length should be at least %ld but is %ld", pos + scidlen, pktlen);
        i_errno = IEQUICPARSEID;
        return -1;
    }
    ngtcp2_cid_init(scid, pkt + pos, scidlen);

    return 0;
}


// Openssl TLS Context and SSL Initialization
static int quic_openssl_init(struct iperf_quic_conn_data *quic_conn_data) {
    int rv;
    struct iperf_test *test = quic_conn_data->sp->test;
    char *sid_ctx;
    BIO *bio_key, *bio_cert;
    EVP_PKEY *key;
    X509 *x509;

    // OpenSSL init
    rv = OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    if (rv != 1) {
        iperf_err(test, "quic_openssl_init: OPENSSL_init_ssl failed: %d", rv);
        i_errno = IEQUICOPENSSLINIT;
        return -1;
    }

    // Set SSL method (client or server)
    quic_conn_data->ssl_ctx = SSL_CTX_new((test->role == 'c') ? TLS_client_method() : TLS_server_method());
    if (!quic_conn_data->ssl_ctx) {
        iperf_err(test, "quic_openssl_init: SSL_CTX_new failed");
        i_errno = IEQUICOPENSSLINIT;
        return -1;
    }

    /* Set SSL trace callback */
    if (test->debug_level >= DEBUG_LEVEL_DEBUG) {
        SSL_CTX_set_msg_callback(quic_conn_data->ssl_ctx, quic_ossl_trace); // SSL_trace);
        SSL_CTX_set_msg_callback_arg(quic_conn_data->ssl_ctx, "SSL_CTX_msg_callback");
    }

    /* Use TLS 1.3 for QUIC */
    SSL_CTX_set_min_proto_version(quic_conn_data->ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(quic_conn_data->ssl_ctx, TLS1_3_VERSION);

    SSL_CTX_set_default_verify_paths(quic_conn_data->ssl_ctx);

    SSL_CTX_set_mode(quic_conn_data->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);

    // --- Load the Private Key from Memory ---
    bio_key = BIO_new_mem_buf(quic_key_pem, -1);
    // BIO_new_mem_buf creates a read-only BIO
    key = PEM_read_bio_PrivateKey(bio_key, NULL, NULL, NULL);
    if (!key) {
        iperf_err(test, "quic_openssl_init: Error loading private key");
        i_errno = IEQUICOPENSSLINIT;
        return 1;
    }
    SSL_CTX_use_PrivateKey(quic_conn_data->ssl_ctx, key);
    BIO_free(bio_key);
    EVP_PKEY_free(key);

    // --- Load the Certificate from Memory ---
    bio_cert = BIO_new_mem_buf(quic_cert_pem, -1);
    x509 = PEM_read_bio_X509(bio_cert, NULL, NULL, NULL);
    if (!x509) {
        iperf_err(test, "quic_openssl_init: Error loading certificate");
        i_errno = IEQUICOPENSSLINIT;
        return 1;
    }
    SSL_CTX_use_certificate(quic_conn_data->ssl_ctx, x509);
    BIO_free(bio_cert);
    X509_free(x509);

    sid_ctx = (test->role == 'c') ? "iperf3 ngtcp2 client" : "iperf3 ngtcp2 server";
    SSL_CTX_set_session_id_context(quic_conn_data->ssl_ctx, (const unsigned char *)sid_ctx, sizeof(sid_ctx) - 1);

// ======= End of SSL CTX settings ==========

    // Create SSL object for the connection
    if (!(quic_conn_data->ssl = SSL_new(quic_conn_data->ssl_ctx))) {
        iperf_err(test, "quic_openssl_init: SSL_new failed");
        i_errno = IEQUICOPENSSLINIT;
        return -1;
    }

    if (test->debug_level >= DEBUG_LEVEL_DEBUG)
        printf("SSL_get_cipher_list=%s\n", SSL_get_cipher_list(quic_conn_data->ssl, 0));

    // Create ngtcp2_crypto_ossl_ctx.
    if ((rv = ngtcp2_crypto_ossl_ctx_new(&quic_conn_data->ng_ssl_ctx, quic_conn_data->ssl)) != 0) {
        iperf_err(test, "quic_openssl_init: ngtcp2_crypto_ossl_ctx_new failed: %d-%s", rv, ngtcp2_strerror(rv));
        SSL_free(quic_conn_data->ssl);
        i_errno = IEQUICOPENSSLINIT;
        return -1;
    }

    return 0;
}


/* iperf_quic_connection_init
 *
 * Initializer for QUIC Connection and connection-streams
 */
int
iperf_quic_connection_init(struct iperf_stream *sp)
{
    int rv, ssl_err;
    ssize_t nread;
    struct iperf_test *test = sp->test;
    uint8_t scidbuf[NGTCP2_MAX_CIDLEN];
    uint64_t rcv_timeout_ms;
    uint8_t buf[65536];
    
    rcv_timeout_ms = (test->settings->rcv_timeout.secs * SEC_TO_mS) + (test->settings->rcv_timeout.usecs / mS_TO_US);

    struct iperf_quic_conn_data *quic_conn_data = &sp->quic_conn_data; // User data for QUIC callbacks
    //quic_conn_data->conn_ref = &quic_conn_data->ng_quic_conn_ref;
    struct ngtcp2_crypto_conn_ref *quic_conn_ref = &quic_conn_data->ng_quic_conn_ref; // Reference to QUIC connection
    quic_conn_ref->user_data = quic_conn_data; // Set user data to our connection data
    quic_conn_ref->get_conn = quic_get_conn; // Set function to retrieve ngtcp2_conn from user data

    ngtcp2_callbacks ng_callbacks; // QUIC callbacks structure
    ngtcp2_settings ng_settings; // QUIC settings structure
    ngtcp2_transport_params ng_params; // QUIC transport parameters structure

    // Initiate user data
    quic_conn_data->sp = sp;
    quic_conn_data->quic_stream_id = -1;

    // Setup ngtcp2 Params, Settings and Callbacks
    quic_init_params(quic_conn_data, &ng_params);
    quic_init_settings(quic_conn_data, &ng_settings);
    quic_init_callbacks(quic_conn_data, &ng_callbacks);

    // Initialize CIDs with random numbers
    if (readentropy(scidbuf, sizeof(scidbuf)) != 0) {
        iperf_err(test, "iperf_quic_connection_init: Failed to generate random DCID for socket=%d", sp->socket);
        i_errno = IEQUICINIT;
        return -1;
    }
    ngtcp2_cid_init(&quic_conn_data->dcid, scidbuf, sizeof(scidbuf));
    if (readentropy(scidbuf, sizeof(scidbuf)) != 0) {
        iperf_err(test, "iperf_quic_connection_init: Failed to generate random SCID for socket=%d", sp->socket);
        i_errno = IEQUICINIT;
        return -1;
    }
    ngtcp2_cid_init(&quic_conn_data->scid, scidbuf, sizeof(scidbuf));

    // print IP/Port - Based on connect_msg() in iperf_api.c
    char ipl[INET6_ADDRSTRLEN], ipr[INET6_ADDRSTRLEN];
    int lport, rport;
    if (getsockdomain(sp->socket) == AF_INET) {
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &sp->local_addr)->sin_addr, ipl, sizeof(ipl));
	    mapped_v4_to_regular_v4(ipl);
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &sp->remote_addr)->sin_addr, ipr, sizeof(ipr));
        mapped_v4_to_regular_v4(ipr);
        lport = ntohs(((struct sockaddr_in *) &sp->local_addr)->sin_port);
        rport = ntohs(((struct sockaddr_in *) &sp->remote_addr)->sin_port);
    } else {
        inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &sp->local_addr)->sin6_addr, ipl, sizeof(ipl));
	    mapped_v4_to_regular_v4(ipl);
        inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &sp->remote_addr)->sin6_addr, ipr, sizeof(ipr));
        mapped_v4_to_regular_v4(ipr);
        lport = ntohs(((struct sockaddr_in6 *) &sp->local_addr)->sin6_port);
        rport = ntohs(((struct sockaddr_in6 *) &sp->remote_addr)->sin6_port);
    }
    if (test->debug_level >= DEBUG_LEVEL_INFO)
        printf("iperf_quic_connection_init: socket=%d, role=%c, local %s:%d <-> remote %s:%d\n", sp->socket, sp->test->role, ipl, lport, ipr, rport);

    // Setup QUIC path (local and remote addresses)
    ngtcp2_path_storage_zero(&quic_conn_data->path);
    ngtcp2_path_storage_init(&quic_conn_data->path,
                            (const ngtcp2_sockaddr *)&sp->local_addr, sp->local_addr_len,
                            (const ngtcp2_sockaddr *)&sp->remote_addr, sp->remote_addr_len,
                            NULL);

    quic_conn_data->path.path.user_data = quic_conn_data;

    quic_conn_data->quic_handshake_completed = 0;
    quic_conn_data->quic_hello_writing = 0;

    //=============== CLIENT ==========================================================
    // Create new Client connection
    if (sp->test->role == 'c') { // Init QUIC Client
        // Init QUIC OpenSSL
        if ((rv = quic_openssl_init(quic_conn_data)) != 0) {
            iperf_err(test, "iperf_quic_connection_init: client quic_openssl_init failed for socket=%d", sp->socket);
            i_errno = IEQUICINIT;
            return -1;
        }

        // Configures OpenSSL to use the ngtcp2-specific key derivation callbacks.
        rv = ngtcp2_crypto_ossl_configure_client_session(quic_conn_data->ssl);
        if (rv != 0) {
            iperf_err(test, "quic_openssl_init: ngtcp2_crypto_ossl_configure_client_session failed for socket=%d: %d-%s", sp->socket, rv, ngtcp2_strerror(rv));
            i_errno = IEQUICINIT;
            return -1;
        }

        // Set ALPN on client: a length-prefixed list of protocols
        unsigned char alpn[1 + 64];
        size_t alpn_len = strlen(IPERF_ALPN_STR);
        alpn[0] = (unsigned char)alpn_len;
        memcpy(alpn + 1, IPERF_ALPN_STR, alpn_len);
        if (SSL_set_alpn_protos(quic_conn_data->ssl, alpn, (unsigned int)(1 + alpn_len)) != 0) {
            iperf_err(test, "client SSL_set_alpn_protos failed for socket=%d", sp->socket);
            i_errno = IEQUICINIT;
            return -1;
        }

        // The dcid parameter is the destination connection ID (DCID).
        // The scid is the source connection ID (SCID) which identifies the client itself.
        rv = ngtcp2_conn_client_new(&quic_conn_data->pconn, &quic_conn_data->dcid, &quic_conn_data->scid,
                &quic_conn_data->path.path, IPERF_QUIC_VERSION, &ng_callbacks, &ng_settings, &ng_params,
                NULL, quic_conn_data);
        if (rv != 0) {
            iperf_err(test, "iperf_quic_connection_init: client ngtcp2_conn_client_new failed for socket=%d %d-%s", sp->socket, rv, ngtcp2_strerror(rv));
            i_errno = IEQUICINIT;
            return -1;
        }

        /* Tie SSL <-> ngtcp2 together via ngtcp2_crypto_conn_ref. */
        //app_conn_ref ref = { quic_conn_data->pconn };
        quic_conn_ref->get_conn = quic_get_conn; // Set function to retrieve ngtcp2_conn from user data
        rv = SSL_set_app_data(quic_conn_data->ssl, quic_conn_ref);
        if (rv != 1) {
            ssl_err = SSL_get_error(quic_conn_data->ssl, rv);
            iperf_err(test, "client SSL_set_app_data failed for socket=%d: %d-%s, errno=%d-%s", sp->socket, ssl_err, ERR_error_string(ssl_err, NULL), errno, strerror(errno));
            i_errno = IEQUICINIT;
            return -1;
        }

        // Start TLS handshake from the client side
        SSL_set_connect_state(quic_conn_data->ssl);
        
        ngtcp2_conn_set_tls_native_handle(quic_conn_data->pconn, quic_conn_data->ng_ssl_ctx);

        // Kick off handshake
        if (quic_flush_tx(quic_conn_data) < 0) {
            iperf_err(test, "iperf_quic_connection_init: client quic_flush_tx failed for socket=%d", sp->socket);
            i_errno = IEQUICINIT;
            return -1;
        }

        // Client main loop
        uint64_t loop_timeout_ms = iperf_time_now_in_ms() +
                                   (test->settings->snd_timeout == 0 ? IPERF_QUIC_DEFAULT_MAX_IDLE_TIMEOUT : test->settings->snd_timeout);
        while(quic_conn_data->quic_stream_id < 0) {  // Client initiation main loop
            if (iperf_time_now_in_ms() > loop_timeout_ms) {
                iperf_err(test, "iperf_quic_connection_init: client timeout waiting for stream to initiate for socket=%d", sp->socket);
                i_errno = IEQUICINIT;
                return -1;
            } 

            // Once handshake completes, open one bidi stream and send message.
            if (quic_conn_data->quic_stream_id < 0 && quic_conn_data->quic_handshake_completed) {
                int64_t sid = -1;
                rv = ngtcp2_conn_open_bidi_stream(quic_conn_data->pconn, &sid, NULL);
                if (rv != 0) {
                    iperf_err(test, "iperf_quic_connection_init: open stream failed for socket=%d: %s", sp->socket, ngtcp2_strerror(rv));
                    return -1;
                }
                quic_conn_data->quic_stream_id = sid;
            }

            (void)quic_flush_tx(quic_conn_data);

            ngtcp2_tstamp now_ns = iperf_time_now_in_ns();
            ngtcp2_tstamp expiry = quic_check_and_handle_expiry(quic_conn_data);
            if (expiry == UINT64_MAX) {
                iperf_err(test, "iperf_quic_connection_init: client quic_check_and_handle_expiry failed for socket=%d %ld-%s", sp->socket, expiry, ngtcp2_strerror(expiry));
                return -1;
            }
            int timeout_ms = 0;
            if (expiry > 0) {
                uint64_t delta = (uint64_t)(expiry - now_ns);
                timeout_ms = (int)(delta / 1000000ull);
                if (timeout_ms < 0) timeout_ms = 0;
            }

            struct pollfd pfd = { .fd = sp->socket, .events = POLLIN, .revents = 0 };
            int pr = poll(&pfd, 1, timeout_ms);
            if (pr < 0) {
                iperf_err(test, "iperf_quic_connection_init: client poll failed for socket=%d; errno=%d-%s", sp->socket, errno, strerror(errno));     
                i_errno = IEQUICINIT;
                return -1;
            }

            now_ns = iperf_time_now_in_ns();

            if (pr == 0) {
                expiry = quic_check_and_handle_expiry(quic_conn_data); // Handle timer expiry
                if (expiry == UINT64_MAX) {
                    iperf_err(test, "iperf_quic_connection_init: client quic_check_and_handle_expiry after poll failed for socket=%d %ld-%s", sp->socket, expiry, ngtcp2_strerror(expiry));
                    return -1;
                }
            }

            if (pfd.revents & POLLIN) {
                uint8_t buf[65536];
                nread = Nread_something_with_timeout(sp->socket, (char *)buf, sizeof(buf), Pquic, rcv_timeout_ms);
                if (nread < 0) {
                    iperf_err(test, "iperf_quic_connection_init: Nread_something_with_timeout failed with timeout for socket=%d; errno=%d-%s", sp->socket, errno, strerror(errno));
                    i_errno = IEQUICINIT;
                    return -1;
                }

                rv = ngtcp2_conn_read_pkt(quic_conn_data->pconn, &quic_conn_data->path.path, NULL, buf, (size_t)nread, now_ns);

                if (rv == NGTCP2_ERR_DRAINING || rv == NGTCP2_ERR_CLOSING) {
                    iperf_err(test, "iperf_quic_connection_init: client ngtcp2_conn_read_pkt: server connection is closing or draining for socket=%d: %s", sp->socket, ngtcp2_strerror(rv));
                    i_errno = IEQUICINIT;
                    return -1;
                }
                if (rv < 0) {
                    iperf_err(test, "iperf_quic_connection_init: client ngtcp2_conn_read_pkt failed for socket=%d: %s", sp->socket, ngtcp2_strerror(rv));
                    i_errno = IEQUICINIT;
                    return -1;
                }
            }
        } // while() client main loop
        
    } // Client

    //=============== SERVER ==========================================================
    // Init QUIC Server
    else if (sp->test->role == 's') {

        quic_conn_data->got_app_data = 0;

        nread = -1;
        rcv_timeout_ms = (test->settings->rcv_timeout.secs * SEC_TO_mS) + (test->settings->rcv_timeout.usecs / mS_TO_US);
        nread = Nread_something_with_timeout(sp->socket, (char *)buf, sizeof(buf), Pquic, rcv_timeout_ms);

        if (nread < 0) {
            iperf_err(test, "iperf_quic_connection_init: reading first packet failed for socket=%d; errno=%d-%s", sp->socket, errno, strerror(errno));
            i_errno = IEQUICINIT;
            return -1;
        }

        // Server must set ngtcp2_transport_params.original_dcid and set ngtcp2_transport_params.original_dcid_present to nonzero.
        // Parse version + client IDs from the first packet
        uint32_t client_version = 0;
        ngtcp2_cid client_dcid, client_scid;
        if (quic_parse_initial_dcid_scid(sp, buf, (size_t)nread, &client_version, &client_dcid, &client_scid) != 0) {
            iperf_err(test, "iperf_quic_connection_init server: could not parse initial header for socket=%d", sp->socket);
            close(sp->socket);
            i_errno = IEQUICINIT;
            return -1;
        }

        // Init QUIC OpenSSL
        if ((rv = quic_openssl_init(quic_conn_data)) != 0) {
            iperf_err(test, "iperf_quic_connection_init: server quic_openssl_init failed for socket=%d", sp->socket);
            i_errno = IEQUICINIT;
            return -1;
        }

        SSL_CTX_set_alpn_select_cb(quic_conn_data->ssl_ctx, quic_alpn_select_cb, NULL);

        /* Start TLS handshake from the server side */
        SSL_set_accept_state(quic_conn_data->ssl);

        // Set original DCID from the client's initial packet
        ng_params.original_dcid_present = 1;
        memcpy(&ng_params.original_dcid, &client_dcid, sizeof(client_dcid));

        // IMPORTANT: Server's DCID for server_new should be Client's SCID
        rv = ngtcp2_conn_server_new(&quic_conn_data->pconn, /*dcid*/&client_scid, /*scid*/&quic_conn_data->scid, &quic_conn_data->path.path,
                NGTCP2_PROTO_VER_V1, &ng_callbacks, &ng_settings, &ng_params,
                NULL, quic_conn_data);
        if (rv != 0) {
            iperf_err(test, "iperf_quic_connection_init: server ngtcp2_conn_server_new failed for socket=%d: %d-%s", sp->socket, rv, ngtcp2_strerror(rv));
            i_errno = IEQUICINIT;
            return -1;
        }

        /* Tie SSL <-> ngtcp2 together via ngtcp2_crypto_conn_ref. */
        quic_conn_ref->get_conn = quic_get_conn; // Set function to retrieve ngtcp2_conn from user data
        rv = SSL_set_app_data(quic_conn_data->ssl, quic_conn_ref);
        if (rv != 1) {
            ssl_err = SSL_get_error(quic_conn_data->ssl, rv);
            iperf_err(test, "server SSL_set_app_data failed for socket=%d: %d-%s, errno=%d-%s", sp->socket, ssl_err, ERR_error_string(ssl_err, NULL), errno, strerror(errno));
            i_errno = IEQUICINIT;
            return -1;
        }

        // Configures OpenSSL to use the ngtcp2-specific key derivation callbacks.
        rv = ngtcp2_crypto_ossl_configure_server_session(quic_conn_data->ssl);
        if (rv != 0) {
            iperf_err(test, "quic_openssl_init : ngtcp2_crypto_ossl_configure_server_session failed for socket=%d: %d-%s", sp->socket, rv, ngtcp2_strerror(rv));
            i_errno = IEQUICINIT;
            return -1;
        }

        ngtcp2_conn_set_tls_native_handle(quic_conn_data->pconn, quic_conn_data->ng_ssl_ctx);

        rv = ngtcp2_conn_read_pkt(quic_conn_data->pconn, &quic_conn_data->path.path, /*pi=*/NULL, buf, (size_t)nread, iperf_time_now_in_ns());
        if (rv < 0) {
            iperf_err(test, "iperf_quic_connection_init: server handshake msg ngtcp2_conn_read_pkt failed for socket=%d: %d-%s", sp->socket, rv, ngtcp2_strerror(rv));
            i_errno = IEQUICINIT;
            return -1;
        }

        (void)quic_flush_tx(quic_conn_data);

        // Server Main Loop
        uint64_t loop_timeout_ms = iperf_time_now_in_ms() +
                                   (test->settings->snd_timeout == 0 ? IPERF_QUIC_DEFAULT_MAX_IDLE_TIMEOUT : test->settings->snd_timeout);
        while (quic_conn_data->quic_stream_id < 0) {
            if (iperf_time_now_in_ms() > loop_timeout_ms) {
                iperf_err(test, "iperf_quic_connection_init: server timeout waiting for stream to initiate for socket=%d", sp->socket);
                i_errno = IEQUICINIT;
                return -1;
            }
            ngtcp2_tstamp now_ns = iperf_time_now_in_ns();
            ngtcp2_tstamp expiry = quic_check_and_handle_expiry(quic_conn_data);
            if (expiry == UINT64_MAX) {
                iperf_err(test, "iperf_quic_connection_init: server quic_check_and_handle_expiry failed for socket=%d %ld-%s", sp->socket, expiry, ngtcp2_strerror(expiry));
                return -1;
            }

            int timeout_ms = 0;
            if (expiry > now_ns) {
                uint64_t delta = (uint64_t)(expiry - now_ns);
                timeout_ms = (int)(delta / 1000000ull);
                if (timeout_ms < 0) timeout_ms = 0;
            }

            struct pollfd pfd = { .fd = sp->socket, .events = POLLIN };
            int pr = poll(&pfd, 1, timeout_ms);
            if (pr < 0) {
                iperf_err(test, "iperf_quic_connection_init: server poll failed for socket=%d; errno=%d-%s", sp->socket, errno, strerror(errno));     
                i_errno = IEQUICINIT;
                return -1;
            }

            now_ns = iperf_time_now_in_ns();
            if (pr == 0) {
                rv = ngtcp2_conn_handle_expiry(quic_conn_data->pconn, now_ns);
                if (rv < 0) {
                    iperf_err(test, "iperf_quic_connection_init: server ngtcp2_conn_handle_expiry failed for socket=%d: %s", sp->socket, ngtcp2_strerror(rv));     
                    i_errno = IEQUICINIT;
                    return -1;
                }
                (void)quic_flush_tx(quic_conn_data);
                continue;
            }

            if (pfd.revents & POLLIN) {
                uint8_t buf[65536];
                nread = Nread_something_with_timeout(sp->socket, (char *)buf, sizeof(buf), Pquic, rcv_timeout_ms);
                if (nread < 0) {
                    iperf_err(test, "iperf_quic_connection_init: server Nread_something_with_timeout failed for socket=%d; errno=%d-%s", sp->socket, errno, strerror(errno));     
                    i_errno = IEQUICINIT;
                    return -1;
                }

                rv = ngtcp2_conn_read_pkt(quic_conn_data->pconn, &quic_conn_data->path.path, NULL, buf, (size_t)nread, now_ns);
                if (rv < 0) {
                    iperf_err(test, "iperf_quic_connection_init: server read_pkt failed for socket=%d: %s", sp->socket, ngtcp2_strerror(rv));
                    i_errno = IEQUICINIT;
                    return -1;
                }

                (void)quic_flush_tx(quic_conn_data);
            }

        } // Server main loop

    } // Server
    
    else {
        iperf_err(test, "iperf_quic_connection_init: Invalid role specified for QUIC protocol initialization for socket=%d", sp->socket);
        i_errno = IEQUICINIT;
        return -1;
    }

    if (test->debug_level >= DEBUG_LEVEL_DEBUG)
        printf("iperf_quic_connection_init: Done QUIC connection init for role=%c, socket=%d, stream_id=%ld\n", sp->test->role, sp->socket, quic_conn_data->quic_stream_id);

    return 0;
}


/* iperf_quic_init
 *
 * initializer for QUIC streams in TEST_START
 */
int
iperf_quic_init(struct iperf_test *test)
{
    int rv;

    if ((rv = ngtcp2_crypto_ossl_init()) != 0) {
        iperf_err(test, "iperf_quic_init: ngtcp2_crypto_ossl_init failed - rv=%d-%s", rv, ngtcp2_strerror(rv));
        i_errno = IEQUICINIT;
        return -1;
    }
    if (test->debug_level >= DEBUG_LEVEL_INFO)
        printf("iperf_quic_init: ngtcp2_crypto_ossl_init success\n");

    return 0;
}


/* iperf_quic_get_tcpinfo
 *
 * Retrieve TCP info from QUIC stream info
 */
void
iperf_quic_get_tcpinfo(struct iperf_stream *sp, struct tcp_info *tcpinfo)
{
    ngtcp2_conn_info cinfo;

    // Get QUIC connection info
    memset(&cinfo, 0, sizeof(cinfo));
    ngtcp2_conn_get_conn_info(sp->quic_conn_data.pconn, &cinfo);

    // Map QUIC connection info to tcp_info structure
    memset(tcpinfo, 0, sizeof(struct tcp_info));
    tcpinfo->tcpi_rto = 0; // Retransmission timeout
    tcpinfo->tcpi_snd_mss = 1; // Max send UDP payload size //TBD: set to 1 to allow get_snd_cwnd() to return the CWND value
    tcpinfo->tcpi_rcv_mss = 0; // Max recv UDP payload size
    tcpinfo->tcpi_snd_ssthresh = cinfo.ssthresh; // Slow start threshold
    tcpinfo->tcpi_bytes_acked = cinfo.bytes_sent - cinfo.bytes_lost - cinfo.bytes_in_flight; // Bytes sent
    tcpinfo->tcpi_bytes_received = cinfo.bytes_recv; // Bytes received
    tcpinfo->tcpi_snd_cwnd = cinfo.cwnd; // Congestion window
    tcpinfo->tcpi_rtt = (uint32_t)(cinfo.latest_rtt / 1000ull); // RTT in micro-sec
    tcpinfo->tcpi_rttvar = (uint32_t)(cinfo.rttvar / 1000ull); // RTT variance in micro-sec
    tcpinfo->tcpi_snd_wnd = 0; // Send window
    tcpinfo->tcpi_total_retrans = cinfo.pkt_lost; // Packets lost
    tcpinfo->tcpi_segs_in = cinfo.pkt_recv; // Packets received
    tcpinfo->tcpi_segs_out = cinfo.pkt_sent; // Packets sent

    return;
}

#endif /* HAVE_QUIC_NGTCP2 */