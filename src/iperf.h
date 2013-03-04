/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_H
#define __IPERF_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "timer.h"
#include "queue.h"
#include "cjson.h"

typedef uint64_t iperf_size_t;

struct iperf_interval_results
{
    iperf_size_t bytes_transferred; /* bytes transfered in this interval */
    struct timeval interval_start_time;
    struct timeval interval_end_time;
    float     interval_duration;
#if defined(linux) || defined(__FreeBSD__)
    struct tcp_info tcpInfo;	/* getsockopt(TCP_INFO) for Linux and FreeBSD */
    int this_retrans;
#else
    /* Just placeholders, never accessed. */
    char *tcpInfo;
    int this_retrans;
#endif
    TAILQ_ENTRY(iperf_interval_results) irlistentries;
    void     *custom_data;
};

struct iperf_stream_result
{
    iperf_size_t bytes_received;
    iperf_size_t bytes_sent;
    iperf_size_t bytes_received_this_interval;
    iperf_size_t bytes_sent_this_interval;
    struct timeval start_time;
    struct timeval end_time;
    TAILQ_HEAD(irlisthead, iperf_interval_results) interval_results;
    void     *data;
};

#define COOKIE_SIZE 37		/* size of an ascii uuid */
struct iperf_settings
{
    int       domain;               /* AF_INET or AF_INET6 */
    int       socket_bufsize;       /* window size for TCP */
    int       blksize;              /* size of read/writes (-l) */
    uint64_t  rate;                 /* target data rate, UDP only */
    int       mss;                  /* for TCP MSS */
    int       ttl;                  /* IP TTL option */
    int       tos;                  /* type of service bit */
    iperf_size_t bytes;             /* number of bytes to send */
    char      unit_format;          /* -f */
};

struct iperf_test;

struct iperf_stream
{
    struct iperf_test* test;

    /* configurable members */
    int       local_port;
    int       remote_port;
    int       socket;
    int       id;
	/* XXX: is settings just a pointer to the same struct in iperf_test? if not, 
		should it be? */
    struct iperf_settings *settings;	/* pointer to structure settings */

    /* non configurable members */
    struct iperf_stream_result *result;	/* structure pointer to result */
    Timer     *send_timer;
    int       udp_green_light;
    int       buffer_fd;	/* data to send, file descriptor */
    char      *buffer;		/* data to send, mmapped */

    /*
     * for udp measurements - This can be a structure outside stream, and
     * stream can have a pointer to this
     */
    int       packet_count;
    double    jitter;
    double    prev_transit;
    int       outoforder_packets;
    int       cnt_error;
    uint64_t  target;

    struct sockaddr_storage local_addr;
    struct sockaddr_storage remote_addr;

    int       (*rcv) (struct iperf_stream * stream);
    int       (*snd) (struct iperf_stream * stream);

//    struct iperf_stream *next;
    SLIST_ENTRY(iperf_stream) streams;

    void     *data;
};

struct protocol {
    int       id;
    char      *name;
    int       (*accept)(struct iperf_test *);
    int       (*listen)(struct iperf_test *);
    int       (*connect)(struct iperf_test *);
    int       (*send)(struct iperf_stream *);
    int       (*recv)(struct iperf_stream *);
    int       (*init)(struct iperf_test *);
    SLIST_ENTRY(protocol) protocols;
};

struct iperf_test
{
    char      role;                             /* c' lient or 's' erver */
    struct protocol *protocol;
    char      state;
    char     *server_hostname;                  /* -c option */
    char     *bind_address;                     /* -B option */
    int       server_port;
    int       duration;                         /* total duration of test (-t flag) */

    int       ctrl_sck;
    int       listener;
    int       prot_listener;

    /* boolean variables for Options */
    int       daemon;                           /* -D option */
    int	      debug;                            /* -d option - debug mode */
    int       no_delay;                         /* -N option */
    int       output_format;                    /* -O option */
    int       reverse;                          /* -R option */
    int       v6domain;                         /* -6 option */
    int	      verbose;                          /* -V option - verbose mode */
    int	      json_output;                      /* -J option - JSON output */
    int	      zerocopy;                         /* -Z option - use sendfile */

    /* Select related parameters */
    int       max_fd;
    fd_set    read_set;                         /* set of read sockets */
    fd_set    write_set;                        /* set of write sockets */

    /* Interval related members */ 
    double    stats_interval;
    double    reporter_interval;
    void      (*stats_callback) (struct iperf_test *);
    void      (*reporter_callback) (struct iperf_test *);
    Timer     *timer;
    int        done;
    Timer     *stats_timer;
    Timer     *reporter_timer;

    double cpu_util;                            /* cpu utilization of the test */
    double remote_cpu_util;                     /* cpu utilization for the remote host/client */

    int       num_streams;                      /* total streams in the test (-P) */

    iperf_size_t bytes_sent;
    char      cookie[COOKIE_SIZE];
//    struct iperf_stream *streams;               /* pointer to list of struct stream */
    SLIST_HEAD(slisthead, iperf_stream) streams;
    struct iperf_settings *settings;

    SLIST_HEAD(plisthead, protocol) protocols;

    /* callback functions */
    void      (*on_new_stream)(struct iperf_stream *);
    void      (*on_test_start)(struct iperf_test *);
    void      (*on_connect)(struct iperf_test *);
    void      (*on_test_finish)(struct iperf_test *);

    /* cJSON handles for use when in -J mode */\
    cJSON *json_top;
    cJSON *json_start;
    cJSON *json_intervals;
    cJSON *json_end;
};

/* default settings */
#define PORT 5201  /* default port to listen on (don't use the same port as iperf2) */
#define uS_TO_NS 1000
#define SEC_TO_US 1000000LL
#define RATE (1024 * 1024) /* 1 Mbps */
#define DURATION 10 /* seconds */

#define SEC_TO_NS 1000000000LL	/* too big for enum/const on some platforms */
#define MAX_RESULT_STRING 4096

/* constants for command line arg sanity checks */
#define MB (1024 * 1024)
#define MAX_TCP_BUFFER (128 * MB)
#define MAX_BLOCKSIZE MB
#define MAX_INTERVAL 60
#define MAX_TIME 3600
#define MAX_MSS (9 * 1024)
#define MAX_STREAMS 128

#endif /* !__IPERF_H */
