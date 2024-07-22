/*
 * iperf, Copyright (c) 2014-2020, 2023, The Regents of the University of
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
#ifndef __IPERF_H
#define __IPERF_H

#include "iperf_config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/socket.h>
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#ifdef HAVE_LINUX_TCP_H
#include <linux/tcp.h>
#else
#include <netinet/tcp.h>
#endif
#include <net/if.h> // for IFNAMSIZ

#if defined(HAVE_CPUSET_SETAFFINITY)
#include <sys/param.h>
#include <sys/cpuset.h>
#endif /* HAVE_CPUSET_SETAFFINITY */

#include "timer.h"
#include "queue.h"
#include "cjson.h"
#include "iperf_time.h"
#include "portable_endian.h"

#if defined(HAVE_SSL)
#include <openssl/bio.h>
#include <openssl/evp.h>
#endif // HAVE_SSL

#include "iperf_pthread.h"

/*
 * Atomic types highly desired, but if not, we approximate what we need
 * with normal integers and warn.
 */
#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#warning "No <stdatomic.h> available."
typedef uint64_t atomic_uint_fast64_t;
#endif // HAVE_STDATOMIC_H

#if !defined(__IPERF_API_H)
typedef uint_fast64_t iperf_size_t;
typedef atomic_uint_fast64_t atomic_iperf_size_t;
#endif // __IPERF_API_H

#if (defined(__vxworks)) || (defined(__VXWORKS__))
typedef unsigned int uint
#endif // __vxworks or __VXWORKS__

struct iperf_interval_results
{
    atomic_iperf_size_t bytes_transferred; /* bytes transferred in this interval */
    struct iperf_time interval_start_time;
    struct iperf_time interval_end_time;
    float     interval_duration;

    /* for UDP */
    int64_t   interval_packet_count;
    int64_t   interval_outoforder_packets;
    int64_t   interval_cnt_error;
    int64_t   packet_count;
    double    jitter;
    int64_t   outoforder_packets;
    int64_t   cnt_error;

    int omitted;
#if (defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && \
	defined(TCP_INFO)
    struct tcp_info tcpInfo; /* getsockopt(TCP_INFO) for Linux, {Free,Net,Open}BSD */
#else
    /* Just placeholders, never accessed. */
    char *tcpInfo;
#endif
    long interval_retrans;
    long snd_cwnd;
    long snd_wnd;
    TAILQ_ENTRY(iperf_interval_results) irlistentries;
    void     *custom_data;
    long rtt;
    long rttvar;
    long pmtu;
};

struct iperf_stream_result
{
    atomic_iperf_size_t bytes_received;
    atomic_iperf_size_t bytes_sent;
    atomic_iperf_size_t bytes_received_this_interval;
    atomic_iperf_size_t bytes_sent_this_interval;
    atomic_iperf_size_t bytes_sent_omit;
    long stream_prev_total_retrans;
    long stream_retrans;
    long stream_max_rtt;
    long stream_min_rtt;
    long stream_sum_rtt;
    int stream_count_rtt;
    long stream_max_snd_cwnd;
    long stream_max_snd_wnd;
    struct iperf_time start_time;
    struct iperf_time end_time;
    struct iperf_time start_time_fixed;
    double sender_time;
    double receiver_time;
    TAILQ_HEAD(irlisthead, iperf_interval_results) interval_results;
    void     *data;
};

#define COOKIE_SIZE 37		/* size of an ascii uuid */
struct iperf_settings
{
    int       domain;               /* AF_INET or AF_INET6 */
    int       socket_bufsize;       /* window size for TCP */
    int       blksize;              /* size of read/writes (-l) */
    iperf_size_t  rate;                 /* target data rate for application pacing*/
    iperf_size_t  bitrate_limit;   /* server's maximum allowed total data rate for all streams*/
    double        bitrate_limit_interval;  /* interval for averaging total data rate */
    int           bitrate_limit_stats_per_interval;     /* calculated number of stats periods for averaging total data rate */
    uint64_t  fqrate;               /* target data rate for FQ pacing*/
    int	      pacing_timer;	    /* pacing timer in microseconds */
    int       burst;                /* packets per burst */
    int       mss;                  /* for TCP MSS */
    int       ttl;                  /* IP TTL option */
    int       tos;                  /* type of service bit */
    int       flowlabel;            /* IPv6 flow label */
    iperf_size_t bytes;             /* number of bytes to send */
    iperf_size_t blocks;            /* number of blocks (packets) to send */
    char      unit_format;          /* -f */
    int       num_ostreams;         /* SCTP initmsg settings */
    int       dont_fragment;        /* Whether to set IP flag Do-Not_Fragment */
#if defined(HAVE_SSL)
    char      *authtoken;           /* Authentication token */
    char      *client_username;
    char      *client_password;
    EVP_PKEY  *client_rsa_pubkey;
#endif // HAVE_SSL
    int	      connect_timeout;	    /* socket connection timeout, in ms */
    int       idle_timeout;         /* server idle time timeout */
    unsigned int snd_timeout; /* Timeout for sending tcp messages in active mode, in us */
    struct iperf_time rcv_timeout;  /* Timeout for receiving messages in active mode, in us */
};

struct iperf_test;

struct iperf_stream
{
    struct iperf_test* test;

    pthread_t thr;
    int       done;

    /* configurable members */
    int       local_port;
    int       remote_port;
    int       socket;
    int       id;
    int       sender;
	/* XXX: is settings just a pointer to the same struct in iperf_test? if not,
		should it be? */
    struct iperf_settings *settings;	/* pointer to structure settings */

    /* non configurable members */
    struct iperf_stream_result *result;	/* structure pointer to result */
    Timer     *send_timer;
    int       green_light;
    int       buffer_fd;	/* data to send, file descriptor */
    char      *buffer;		/* data to send, mmapped */
    int       pending_size;     /* pending data to send */
    int       diskfile_fd;	/* file to send, file descriptor */
    int	      diskfile_left;	/* remaining file data on disk */

    /*
     * for udp measurements - This can be a structure outside stream, and
     * stream can have a pointer to this
     */
    int64_t   packet_count;
    int64_t   peer_packet_count;
    int64_t   peer_omitted_packet_count;
    int64_t   omitted_packet_count;
    double    jitter;
    double    prev_transit;
    int64_t   outoforder_packets;
    int64_t   omitted_outoforder_packets;
    int64_t   cnt_error;
    int64_t   omitted_cnt_error;
    uint64_t  target;

    struct sockaddr_storage local_addr;
    struct sockaddr_storage remote_addr;

    int       (*rcv) (struct iperf_stream * stream);
    int       (*snd) (struct iperf_stream * stream);

    /* chained send/receive routines for -F mode */
    int       (*rcv2) (struct iperf_stream * stream);
    int       (*snd2) (struct iperf_stream * stream);

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

struct iperf_textline {
    char *line;
    TAILQ_ENTRY(iperf_textline) textlineentries;
};

struct xbind_entry {
    char *name;
    struct addrinfo *ai;
    TAILQ_ENTRY(xbind_entry) link;
};

enum iperf_mode {
	SENDER = 1,
	RECEIVER = 0,
	BIDIRECTIONAL = -1
};

enum debug_level {
    DEBUG_LEVEL_ERROR = 1,
    DEBUG_LEVEL_WARN = 2,
    DEBUG_LEVEL_INFO = 3,
    DEBUG_LEVEL_DEBUG = 4,
    DEBUG_LEVEL_MAX = 4
};


struct iperf_test
{
    pthread_mutex_t print_mutex;

    char      role;                             /* 'c' lient or 's' erver */
    enum iperf_mode mode;
    int       sender_has_retransmits;
    int       other_side_has_retransmits;       /* used if mode == BIDIRECTIONAL */
    struct protocol *protocol;
    signed char state;
    char     *server_hostname;                  /* -c option */
    char     *tmp_template;
    char     *bind_address;                     /* first -B option */
    char     *bind_dev;                         /* bind to network device */
    TAILQ_HEAD(xbind_addrhead, xbind_entry) xbind_addrs; /* all -X opts */
    int       bind_port;                        /* --cport option */
    int       server_port;
    int       omit;                             /* duration of omit period (-O flag) */
    int       duration;                         /* total duration of test (-t flag) */
    char     *diskfile_name;			/* -F option */
    int       affinity, server_affinity;	/* -A option */
#if defined(HAVE_CPUSET_SETAFFINITY)
    cpuset_t cpumask;
#endif /* HAVE_CPUSET_SETAFFINITY */
    char     *title;				/* -T option */
    char     *extra_data;			/* --extra-data */
    char     *congestion;			/* -C option */
    char     *congestion_used;			/* what was actually used */
    char     *remote_congestion_used;		/* what the other side used */
    char     *pidfile;				/* -P option */

    char     *logfile;				/* --logfile option */
    FILE     *outfile;

    int       ctrl_sck;
    int       mapped_v4;
    int       listener;
    int       prot_listener;

    int	      ctrl_sck_mss;			/* MSS for the control channel */

#if defined(HAVE_SSL)
    char      *server_authorized_users;
    EVP_PKEY  *server_rsa_private_key;
    int       server_skew_threshold;
    int       use_pkcs1_padding;
#endif // HAVE_SSL

    /* boolean variables for Options */
    int       daemon;                           /* -D option */
    int       one_off;                          /* -1 option */
    int       no_delay;                         /* -N option */
    int       reverse;                          /* -R option */
    int       bidirectional;                    /* --bidirectional */
    int	      verbose;                          /* -V option - verbose mode */
    int	      json_output;                      /* -J option - JSON output */
    int	      json_stream;                      /* --json-stream */
    int	      zerocopy;                         /* -Z option - use sendfile */
    int       debug;				/* -d option - enable debug */
    enum      debug_level debug_level;          /* -d option option - level of debug messages to show */
    int	      get_server_output;		/* --get-server-output */
    int	      udp_counters_64bit;		/* --use-64-bit-udp-counters */
    int       forceflush; /* --forceflush - flushing output at every interval */
    int	      multisend;
    int	      repeating_payload;                /* --repeating-payload */
    int       timestamps;			/* --timestamps */
    char     *timestamp_format;

    char     *json_output_string; /* rendered JSON output if json_output is set */
    /* Select related parameters */
    int       max_fd;
    fd_set    read_set;                         /* set of read sockets */

    /* Interval related members */
    int       omitting;
    double    stats_interval;
    double    reporter_interval;
    void      (*stats_callback) (struct iperf_test *);
    void      (*reporter_callback) (struct iperf_test *);
    Timer     *omit_timer;
    Timer     *timer;
    int        done;
    Timer     *stats_timer;
    Timer     *reporter_timer;

    double cpu_util[3];                            /* cpu utilization of the test - total, user, system */
    double remote_cpu_util[3];                     /* cpu utilization for the remote host/client - total, user, system */

    int       num_streams;                      /* total streams in the test (-P) */

    atomic_iperf_size_t bytes_sent;
    atomic_iperf_size_t blocks_sent;

    atomic_iperf_size_t bytes_received;
    atomic_iperf_size_t blocks_received;

    iperf_size_t bitrate_limit_stats_count;               /* Number of stats periods accumulated for server's total bitrate average */
    iperf_size_t *bitrate_limit_intervals_traffic_bytes;  /* Pointer to a cyclic array that includes the last interval's bytes transferred */
    iperf_size_t bitrate_limit_last_interval_index;       /* Index of the last interval traffic inserted into the cyclic array */
    int          bitrate_limit_exceeded;                  /* Set by callback routine when average data rate exceeded the server's bitrate limit */

    int server_last_run_rc;                      /* Save last server run rc for next test */
    uint server_forced_idle_restarts_count;      /* count number of forced server restarts to make sure it is not stack */
    uint server_forced_no_msg_restarts_count;    /* count number of forced server restarts to make sure it is not stack */
    uint server_test_number;                     /* count number of tests performed by a server */

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
    cJSON *json_connected;
    cJSON *json_intervals;
    cJSON *json_end;

    /* Server output (use on client side only) */
    char *server_output_text;
    cJSON *json_server_output;

    /* Server output (use on server side only) */
    TAILQ_HEAD(iperf_textlisthead, iperf_textline) server_output_list;

};

/* default settings */
#define PORT 5201  /* default port to listen on (don't use the same port as iperf2) */
#define uS_TO_NS 1000
#define mS_TO_US 1000
#define SEC_TO_mS 1000
#define SEC_TO_US 1000000LL
#define UDP_RATE (1024 * 1024) /* 1 Mbps */
#define OMIT 0 /* seconds */
#define DURATION 10 /* seconds */

#define SEC_TO_NS 1000000000LL	/* too big for enum/const on some platforms */
#define MAX_RESULT_STRING 4096

#define UDP_BUFFER_EXTRA 1024

/* constants for command line arg sanity checks */
#define MB (1024 * 1024)
#define MAX_TCP_BUFFER (512 * MB)
#define MAX_BLOCKSIZE MB
/* Minimum size UDP send is the size of two 32-bit ints followed by a 64-bit int */
#define MIN_UDP_BLOCKSIZE (4 + 4 + 8)
/* Maximum size UDP send is (64K - 1) - IP and UDP header sizes */
#define MAX_UDP_BLOCKSIZE (65535 - 8 - 20)
#define MIN_INTERVAL 0.1
#define MAX_INTERVAL 60.0
#define MAX_TIME 86400
#define MAX_BURST 1000
#define MAX_MSS (9 * 1024)
#define MAX_STREAMS 128

#define TIMESTAMP_FORMAT "%c "

extern int gerror; /* error value from getaddrinfo(3), for use in internal error handling */

/* UDP "connect" message and reply (textual value for Wireshark, etc. readability - legacy was numeric) */

#if BYTE_ORDER == BIG_ENDIAN
#define UDP_CONNECT_MSG 0x39383736
#define UDP_CONNECT_REPLY 0x36373839
#define LEGACY_UDP_CONNECT_REPLY 0xb168de3a
#else
#define UDP_CONNECT_MSG 0x36373839          // "6789" - legacy value was 123456789
#define UDP_CONNECT_REPLY 0x39383736        // "9876" - legacy value was 987654321
#define LEGACY_UDP_CONNECT_REPLY 987654321  // Old servers may still reply with the legacy value
#endif

/* In Reverse mode, maximum number of packets to wait for "accept" response - to handle out of order packets */
#define MAX_REVERSE_OUT_OF_ORDER_PACKETS 2

#endif /* !__IPERF_H */
