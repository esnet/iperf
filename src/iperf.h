

/*
   Copyright (c) 2009, The Regents of the University of California, through
   Lawrence Berkeley National Laboratory (subject to receipt of any required
   approvals from the U.S. Dept. of Energy).  All rights reserved.
*/

#ifndef        IPERF_H
#define        IPERF_H

typedef uint64_t iperf_size_t;

struct iperf_interval_results
{
    iperf_size_t bytes_transferred;
    struct timeval interval_time;
    float     interval_duration;
#if defined(linux) || defined(__FreeBSD__)
    struct tcp_info tcpInfo;	/* getsockopt(TCP_INFO) results here for
				 * Linux and FreeBSD stored here */
#else
    char *tcpInfo;	/* just a placeholder */
#endif
    struct iperf_interval_results *next;
    void     *custom_data;
};

struct iperf_stream_result
{
    iperf_size_t bytes_received;
    iperf_size_t bytes_sent;
    struct timeval start_time;
    struct timeval end_time;
    struct iperf_interval_results *interval_results;
    void     *data;
};

#define COOKIE_SIZE 37  /* size of an ascii uuid */
struct iperf_settings
{
    int       socket_bufsize; /* window size for TCP */
    int       blksize;		/* size of read/writes (-l) */
    uint64_t  rate;		/* target data rate, UDP only */
    int       mss;		/* for TCP MSS */
    int       ttl;
    int       tos;
    iperf_size_t bytes;		/* -n option */
    char      unit_format;	/* -f */
    /* XXX: not sure about this design: everything else is this struct is static state, 
	but the last 2 are dymanic state. Should they be in iperf_stream instead? */
    int       state;		/* This is state of a stream/test */
    char      cookie[COOKIE_SIZE];	
};

struct iperf_stream
{
    /* configurable members */
    int       local_port;
    int       remote_port;
	/* XXX: is this just a pointer to the same struct in iperf_test? if not, 
		should it be? */
    struct iperf_settings *settings;	/* pointer to structure settings */

    /* non configurable members */
    struct iperf_stream_result *result;	/* structure pointer to result */
    int       socket;
    struct timer *send_timer;
    char     *buffer;		/* data to send */

    /*
     * for udp measurements - This can be a structure outside stream, and
     * stream can have a pointer to this
     */
    int       packet_count;
    int       stream_id;	/* stream identity for UDP mode */
    double    jitter;
    double    prev_transit;
    int       outoforder_packets;
    int       cnt_error;
    uint64_t  target;

    struct sockaddr_storage local_addr;
    struct sockaddr_storage remote_addr;

    int       (*rcv) (struct iperf_stream * stream);
    int       (*snd) (struct iperf_stream * stream);
    int       (*update_stats) (struct iperf_stream * stream);

    struct iperf_stream *next;

    void     *data;
};

struct iperf_test
{
    char      role;		/* c' lient or 's' erver */
    int       protocol;
    char     *server_hostname;	/* -c option */
    int       server_port;
    int       duration;		/* total duration of test (-t flag) */
    int       listener_sock_tcp;
    int       listener_sock_udp;

    /* boolen variables for Options */
    int       daemon;		/* -D option */
    int       no_delay;		/* -N option */
    int       print_mss;	/* -m option */
    int       domain;		/* -V option */

    /* Select related parameters */
    int       max_fd;
    fd_set    read_set;
    fd_set    temp_set;
    fd_set    write_set;

    int       (*accept) (struct iperf_test *);
    struct iperf_stream *(*new_stream) (struct iperf_test *);
    int       stats_interval;	/* time interval to gather stats (-i) */
    void     *(*stats_callback) (struct iperf_test *);	/* callback function
							 * pointer for stats */
    int       reporter_interval;/* time interval for reporter */
    char     *(*reporter_callback) (struct iperf_test *);	/* callback function
								 * pointer for reporter */
    int       reporter_fd;	/* file descriptor for reporter */
    int       num_streams;	/* total streams in the test (-P) */
    int       tcp_info;		/* display getsockopt(TCP_INFO) results */
    struct iperf_stream *streams;	/* pointer to list of struct stream */
    struct iperf_settings *default_settings;
};

struct udp_datagram
{
    int       state;
    int       stream_id;
    int       packet_count;
    struct timeval sent_time;
};

struct param_exchange
{
    int       state;
    int       stream_id;
    int       blksize;
    int       recv_window;
    int       send_window;
    int       mss;
    char      format;
    char      cookie[COOKIE_SIZE]; 
};

enum
{
    /* default settings */
    Ptcp = SOCK_STREAM,
    Pudp = SOCK_DGRAM,
    PORT = 5001,  /* default port to listen on */
    uS_TO_NS = 1000,
    SEC_TO_US = 1000000,
    RATE = 1024 * 1024, /* 1 Mbps */
    DURATION = 5, /* seconds */
    DEFAULT_UDP_BLKSIZE = 1450, /* 1 packet per ethernet frame, IPV6 too */
    DEFAULT_TCP_BLKSIZE = 128 * 1024,  /* default read/write block size */

    /* other useful constants */
    TEST_START = 1,
    TEST_RUNNING = 2,
    RESULT_REQUEST = 3,
    RESULT_RESPOND = 4,
    TEST_END = 5,
    STREAM_BEGIN = 6,
    STREAM_RUNNING = 7,
    STREAM_END = 8,
    ALL_STREAMS_END = 9,
    PARAM_EXCHANGE = 10,
    PARAM_EXCHANGE_ACK = 11,
    ACCESS_DENIED = -1,
};

#define SEC_TO_NS 1000000000	/* too big for enum on some platforms */
#define MAX_RESULT_STRING 4096

#endif  /* IPERF_API_H */

