/*
 * iperfjd -- greatly simplified version of iperf with the same interface
 * semantics
 *
 * jdugan -- 24 Jan 2009
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <stdint.h>

#include <sys/time.h>

#include "timer.h"
#include "net.h"
#include "tcp_window_size.h"

enum {
    Mundef = 0,
    Mclient,
    Mserver,

    Ptcp = SOCK_STREAM,
    Pudp = SOCK_DGRAM,

    uS_TO_NS = 1000,

    DEFAULT_UDP_BUFSIZE = 1470,
    DEFAULT_TCP_BUFSIZE = 8192
};
#define SEC_TO_NS 1000000000 /* too big for enum on some platforms */

struct stream
{
    int sock;  /* local socket */
    struct sockaddr_in local; /* local address */
    struct sockaddr_in peer; /* peer address */

    uint64_t bytes_in;
    uint64_t bytes_out;

    pthread_t thread;

    char local_addr[512], peer_addr[512];

    void *stats; /* ptr to protocol specific stats */

    void *(*server)(void *sp);
    void *(*client)(void *sp);

    struct stream *next;
};

void *udp_client_thread(struct stream *sp);
void *udp_server_thread(struct stream *sp);
void *tcp_client_thread(struct stream *sp);
void *tcp_server_thread(struct stream *sp);

static struct option longopts[] =
{
    { "client",         required_argument,      NULL,   'c' },
    { "server",         no_argument,            NULL,   's' },
    { "time",           required_argument,      NULL,   't' },
    { "port",           required_argument,      NULL,   'p' },
    { "parallel",       required_argument,      NULL,   'P' },
    { "udp",            no_argument,            NULL,   'u' },
    { "bandwidth",      required_argument,      NULL,   'b' },
    { "length",         required_argument,      NULL,   'l' },
    { "window",         required_argument,      NULL,   'w' },
    { NULL,             0,                      NULL,   0   }
};

struct settings
{
    char mode;
    char proto;
    char *client;
    int port;
    int sock;
    uint64_t bw;
    int duration;
    int threads;
    long int bufsize;
    long int window;
    struct sockaddr_in sa;
};

struct settings settings =
{
    Mundef,
    Ptcp,
    NULL,
    5001,
    -1,
    1000000,
    10,
    1,
    DEFAULT_UDP_BUFSIZE,
    1024*1024
};

struct stream *streams;  /* head of list of streams */
int done = 0;

struct stream *
new_stream(int s)
{
    struct stream *sp;
    socklen_t len;

    sp = (struct stream *) malloc(sizeof(struct stream));
    if(!sp) {
        perror("malloc");
        return(NULL);
    }

    memset(sp, 0, sizeof(struct stream));

    sp->sock = s;

    len = sizeof sp->local;
    if(getsockname(sp->sock, (struct sockaddr *) &sp->local, &len) < 0) {
        perror("getsockname");
        free(sp);
        return(NULL);
    }

    if(inet_ntop(AF_INET, (void *) &sp->local.sin_addr,
                (void *) &sp->local_addr, 512) == NULL) {
        perror("inet_pton");
    }

    if(getpeername(sp->sock, (struct sockaddr *) &sp->peer, &len) < 0) {
        perror("getpeername");
        free(sp);
        return(NULL);
    }

    if(inet_ntop(AF_INET, (void *) &sp->peer.sin_addr,
                (void *) &sp->peer_addr, 512) == NULL) {
        perror("inet_pton");
    }

    switch (settings.proto) {
        case Ptcp:
            sp->client = (void *) tcp_client_thread;
            sp->server = (void *) tcp_server_thread;
            break;
        case Pudp:
            sp->client = (void *) udp_client_thread;
            sp->server = (void *) udp_server_thread;
            break;
        default:
            assert(0);
            break;
    }

    if(set_tcp_windowsize(sp->sock, settings.window, 
                settings.mode == Mserver ? SO_RCVBUF : SO_SNDBUF) < 0)
        fprintf(stderr, "unable to set window size\n");

    int x;
    x = getsock_tcp_windowsize(sp->sock, SO_RCVBUF);
    if(x < 0)
        perror("SO_RCVBUF");

    printf("RCV: %d\n", x);

    x = getsock_tcp_windowsize(sp->sock, SO_SNDBUF);
    if(x < 0)
        perror("SO_SNDBUF");

    printf("SND: %d\n", x);

    return(sp);
}

void
add_stream(struct stream *sp)
{
    struct stream *n;

    if(!streams)
        streams = sp;
    else {
        n = streams;
        while(n->next)
            n = n->next;
        n->next = sp;
    }
}

void
free_stream(struct stream *sp)
{
    free(sp);
}

void connect_msg(struct stream *sp)
{
    char *ipl, *ipr;

    ipl = (char *) &sp->local.sin_addr;
    ipr = (char *) &sp->peer.sin_addr;

    printf("[%3d] local %s port %d connected with %s port %d\n",
            sp->sock, 
            sp->local_addr, htons(sp->local.sin_port),
            sp->peer_addr, htons(sp->peer.sin_port));
}

void *
udp_client_thread(struct stream *sp)
{
    int i;
    int64_t delayns, adjustns, dtargns;
    char *buf;
    struct timeval before, after;

    buf = (char *) malloc(settings.bufsize);
    if(!buf) {
        perror("malloc: unable to allocate transmit buffer");
        pthread_exit(NULL);
    }

    for(i=0; i < settings.bufsize; i++)
        buf[i] = i % 37;

    dtargns = (int64_t) settings.bufsize * SEC_TO_NS * 8;
    dtargns /= settings.bw;

    assert(dtargns != 0);

    if(gettimeofday(&before, 0) < 0) {
        perror("gettimeofday");
    }

    delayns = dtargns;
    adjustns = 0;
    printf("%lld adj %lld delay\n", adjustns, delayns);
    while(!done) {
        send(sp->sock, buf, settings.bufsize, 0);
        sp->bytes_out += settings.bufsize;

        if(delayns > 0)
            delay(delayns);

        if(gettimeofday(&after, 0) < 0) {
            perror("gettimeofday");
        }

        adjustns = dtargns;
        adjustns += (before.tv_sec - after.tv_sec) * SEC_TO_NS;
        adjustns += (before.tv_usec - after.tv_usec) * uS_TO_NS;

        if( adjustns > 0 || delayns > 0) {
            //printf("%lld adj %lld delay\n", adjustns, delayns);
            delayns += adjustns;
        }

        memcpy(&before, &after, sizeof before);
    }

    /* a 0 byte packet is the server's cue that we're done */
    send(sp->sock, buf, 0, 0);

    /* XXX: wait for response with server counts */

    printf("%llu bytes sent\n", sp->bytes_out);

    close(sp->sock);
    pthread_exit(NULL);
}

void *
udp_server_thread(struct stream *sp)
{
    char *buf;
    ssize_t sz;

    buf = (char *) malloc(settings.bufsize);
    if(!buf) {
        perror("malloc: unable to allocate receive buffer");
        pthread_exit(NULL);
    }

    while((sz = recv(sp->sock, buf, settings.bufsize, 0)) > 0) {
        sp->bytes_in += sz;
    }

    close(sp->sock);
    printf("%llu bytes received %g\n", sp->bytes_in, (sp->bytes_in * 8.0)/(settings.duration*1000.0*1000.0));
    pthread_exit(NULL);
}

void
udp_report(int final)
{
}

void
tcp_report(int final)
{
}

void *
tcp_client_thread(struct stream *sp)
{
    int i;
    char *buf;

    buf = (char *) malloc(settings.bufsize);
    if(!buf) {
        perror("malloc: unable to allocate transmit buffer");
        pthread_exit(NULL);
    }

    printf("window: %d\n", getsock_tcp_windowsize(sp->sock, SO_SNDBUF));

    for(i=0; i < settings.bufsize; i++)
        buf[i] = i % 37;

    while(!done) {
        send(sp->sock, buf, settings.bufsize, 0);
        sp->bytes_out += settings.bufsize;
    }

    /* a 0 byte packet is the server's cue that we're done */
    send(sp->sock, buf, 0, 0);

    /* XXX: wait for response with server counts */

    printf("%llu bytes sent\n", sp->bytes_out);

    close(sp->sock);
    pthread_exit(NULL);
}

void *
tcp_server_thread(struct stream *sp)
{
    char *buf;
    ssize_t sz;

    buf = (char *) malloc(settings.bufsize);
    if(!buf) {
        perror("malloc: unable to allocate receive buffer");
        pthread_exit(NULL);
    }

    printf("window: %d\n", getsock_tcp_windowsize(sp->sock, SO_RCVBUF));

    while( (sz = recv(sp->sock, buf, settings.bufsize, 0)) > 0) {
        sp->bytes_in += sz;
    }

    close(sp->sock);
    printf("%llu bytes received %g\n", sp->bytes_in, (sp->bytes_in * 8.0)/(settings.duration*1000.0*1000.0));
    pthread_exit(NULL);
}

int
client(void)
{
    int s, i;
    struct stream *sp;
    struct timer *timer;

    for(i = 0; i < settings.threads; i++) {
        s = netdial(settings.proto, settings.client, settings.port);
        if(s < 0) {
            fprintf(stderr, "netdial failed\n");
            return -1;
        }
    
        set_tcp_windowsize(s, settings.window, SO_SNDBUF);

        if(s < 0)
            return -1;

        sp = new_stream(s);
        add_stream(sp);
        connect_msg(sp);
        pthread_create(&sp->thread, NULL, sp->client, (void *) sp);
    }

    timer = new_timer(settings.duration, 0);

    while(!timer->expired(timer))
        sleep(settings.duration);

    done = 1;

    /* XXX: report */
    sp = streams;
    do {
        pthread_join(sp->thread, NULL);
        sp = sp->next;
    } while (sp);

    return 0;
}

int
server()
{
    int s, cs, sz;
    struct stream *sp;
    struct sockaddr_in sa_peer;
    socklen_t len;
    char buf[settings.bufsize];

    s = netannounce(settings.proto, NULL, settings.port);
    if(s < 0)
        return -1;

    if(set_tcp_windowsize(s, settings.window, SO_RCVBUF) < 0) {
        perror("unable to set window");
        return -1;
    }

    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", settings.port);
    int x;
    if((x = getsock_tcp_windowsize(s, SO_RCVBUF)) < 0) 
        perror("SO_RCVBUF");

    printf("%s: %d\n",
            settings.proto == Ptcp ? "TCP window size" : "UDP buffer size", x);
    printf("-----------------------------------------------------------\n");

    len = sizeof sa_peer;
    while(1) {
        if (Ptcp == settings.proto) {
            if( (cs = accept(s, (struct sockaddr *) &sa_peer, &len)) < 0) {
                perror("accept");
                return -1;
            }
            sp = new_stream(cs);
        } else if (Pudp == settings.proto) {
            sz = recvfrom(s, buf, settings.bufsize, 0, (struct sockaddr *) &sa_peer, &len);
            if(!sz)
                break;

            if(connect(s, (struct sockaddr *) &sa_peer, len) < 0) {
                perror("connect");
                return -1;
            }

            sp = new_stream(s);
            sp->bytes_in += sz;

            s = netannounce(settings.proto, NULL, settings.port);
            if(s < 0) {
                return -1;
            }
        }

        connect_msg(sp);
        pthread_create(&sp->thread, NULL, sp->server, (void *) sp);
    }

    return 0;
}

int
main(int argc, char **argv)
{
    int rc;
    char ch;

    while( (ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:", longopts, NULL)) != -1 )
        switch (ch) {
            case 'c':
                settings.mode = Mclient;
                settings.client = malloc(strlen(optarg));
                strcpy(settings.client, optarg);
                break;
            case 'p':
                settings.port = atoi(optarg);
                break;
            case 's':
                settings.mode = Mserver;
                break;
            case 't':
                settings.duration = atoi(optarg);
                break;
            case 'u':
                settings.proto = Pudp;
                break;
            case 'P':
                settings.threads = atoi(optarg);
                break;
            case 'b':
                settings.bw = atoll(optarg);
                break;
            case 'l':
                settings.bufsize = atol(optarg);
                break;
            case 'w':
                settings.window = atoi(optarg);
                break;
        }

    if (settings.proto == Ptcp && settings.bufsize == DEFAULT_UDP_BUFSIZE)
        settings.bufsize = DEFAULT_TCP_BUFSIZE; /* XXX: this might be evil */

    switch (settings.mode) {
        case Mclient:
            rc = client();
            break;
        case Mserver:
            rc = server();
            break;
        case Mundef:
            /* FALLTHRU */
        default:
            printf("must specify one of -s or -c\n");
            rc = -1;
            break;
    }

    return rc;
}
