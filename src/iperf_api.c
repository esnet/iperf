/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <setjmp.h>

#include "net.h"
#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "timer.h"

#include "cjson.h"
#include "units.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "locale.h"

jmp_buf env;            /* to handle longjmp on signal */


/* Forwards. */
static int send_parameters(struct iperf_test *test);
static int get_parameters(struct iperf_test *test);
static int send_results(struct iperf_test *test);
static int get_results(struct iperf_test *test);
static int JSON_write(int fd, cJSON *json);
static cJSON *JSON_read(int fd);


/*************************** Print usage functions ****************************/

void
usage()
{
    fprintf(stderr, usage_short);
}


void
usage_long()
{
    fprintf(stderr, usage_long1);
    fprintf(stderr, usage_long2);
}


void warning(char *str)
{
    fprintf(stderr, "warning: %s\n", str);
}


/************** Getter routines for some fields inside iperf_test *************/

int
iperf_get_test_duration( struct iperf_test* ipt )
{
    return ipt->duration;
}

uint64_t
iperf_get_test_rate( struct iperf_test* ipt )
{
    return ipt->settings->rate;
}

char
iperf_get_test_role( struct iperf_test* ipt )
{
    return ipt->role;
}

int
iperf_get_test_blksize( struct iperf_test* ipt )
{
    return ipt->settings->blksize;
}

int
iperf_get_test_socket_bufsize( struct iperf_test* ipt )
{
    return ipt->settings->socket_bufsize;
}

double
iperf_get_test_reporter_interval( struct iperf_test* ipt )
{
    return ipt->reporter_interval;
}

double
iperf_get_test_stats_interval( struct iperf_test* ipt )
{
    return ipt->stats_interval;
}

int
iperf_get_test_num_streams( struct iperf_test* ipt )
{
    return ipt->num_streams;
}

int
iperf_get_test_server_port( struct iperf_test* ipt )
{
    return ipt->server_port;
}

char*
iperf_get_test_server_hostname( struct iperf_test* ipt )
{
    return ipt->server_hostname;
}

int
iperf_get_test_protocol_id( struct iperf_test* ipt )
{
    return ipt->protocol->id;
}

/************** Setter routines for some fields inside iperf_test *************/

void
iperf_set_test_duration( struct iperf_test* ipt, int duration )
{
    ipt->duration = duration;
}

void
iperf_set_test_reporter_interval( struct iperf_test* ipt, double reporter_interval )
{
    ipt->reporter_interval = reporter_interval;
}

void
iperf_set_test_stats_interval( struct iperf_test* ipt, double stats_interval )
{
    ipt->stats_interval = stats_interval;
}

void
iperf_set_test_state( struct iperf_test* ipt, char state )
{
    ipt->state = state;
}

void
iperf_set_test_blksize( struct iperf_test* ipt, int blksize )
{
    ipt->settings->blksize = blksize;
}

void
iperf_set_test_rate( struct iperf_test* ipt, uint64_t rate )
{
    ipt->settings->rate = rate;
}

void
iperf_set_test_server_port( struct iperf_test* ipt, int server_port )
{
    ipt->server_port = server_port;
}

void
iperf_set_test_socket_bufsize( struct iperf_test* ipt, int socket_bufsize )
{
    ipt->settings->socket_bufsize = socket_bufsize;
}

void
iperf_set_test_num_streams( struct iperf_test* ipt, int num_streams )
{
    ipt->num_streams = num_streams;
}

void
iperf_set_test_role( struct iperf_test* ipt, char role )
{
    ipt->role = role;
}

void
iperf_set_test_server_hostname( struct iperf_test* ipt, char* server_hostname )
{
    ipt->server_hostname = server_hostname;
}

/********************** Get/set test protocol structure ***********************/

struct protocol *
get_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id)
            break;
    }

    if (prot == NULL)
        i_errno = IEPROTOCOL;

    return (prot);
}

int
set_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot = NULL;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id) {
            test->protocol = prot;
            return (0);
        }
    }

    i_errno = IEPROTOCOL;

    return (-1);
}


/************************** Iperf callback functions **************************/

void
iperf_on_new_stream(struct iperf_stream *sp)
{
    connect_msg(sp);
}

void
iperf_on_test_start(struct iperf_test *test)
{
    if (test->verbose) {
        if (test->settings->bytes) {
            printf(test_start_bytes, test->protocol->name, test->num_streams,
                test->settings->blksize, test->settings->bytes);
        } else {
            printf(test_start_time, test->protocol->name, test->num_streams,
                test->settings->blksize, test->duration);
        }
    }
}

void
iperf_on_connect(struct iperf_test *test)
{
    char ipr[INET6_ADDRSTRLEN];
    struct sockaddr_storage temp;
    socklen_t len;
    int domain, opt;

    if (test->role == 'c') {
        printf("Connecting to host %s, port %d\n", test->server_hostname,
            test->server_port);
    } else {
        domain = test->settings->domain;
        len = sizeof(temp);
        getpeername(test->ctrl_sck, (struct sockaddr *) &temp, &len);
        if (domain == AF_INET) {
            inet_ntop(domain, &((struct sockaddr_in *) &temp)->sin_addr, ipr, sizeof(ipr));
            printf("Accepted connection from %s, port %d\n", ipr, ntohs(((struct sockaddr_in *) &temp)->sin_port));
        } else {
            inet_ntop(domain, &((struct sockaddr_in6 *) &temp)->sin6_addr, ipr, sizeof(ipr));
            printf("Accepted connection from %s, port %d\n", ipr, ntohs(((struct sockaddr_in6 *) &temp)->sin6_port));
        }
    }
    if (test->verbose) {
        printf("      Cookie: %s\n", test->cookie);
        if (test->protocol->id == SOCK_STREAM) {
            if (test->settings->mss) {
                printf("      TCP MSS: %d\n", test->settings->mss);
            } else {
                len = sizeof(opt);
                getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len);
                printf("      TCP MSS: %d (default)\n", opt);
            }
        }

    }
}

void
iperf_on_test_finish(struct iperf_test *test)
{
    if (test->verbose) {
        printf("Test Complete. Summary Results:\n");
    }
}


/******************************************************************************/

int
iperf_parse_arguments(struct iperf_test *test, int argc, char **argv)
{
    static struct option longopts[] =
    {
        {"client", required_argument, NULL, 'c'},
        {"server", no_argument, NULL, 's'},
        {"time", required_argument, NULL, 't'},
        {"port", required_argument, NULL, 'p'},
        {"parallel", required_argument, NULL, 'P'},
        {"udp", no_argument, NULL, 'u'},
        {"bind", required_argument, NULL, 'B'},
        {"bandwidth", required_argument, NULL, 'b'},
        {"length", required_argument, NULL, 'l'},
        {"window", required_argument, NULL, 'w'},
        {"interval", required_argument, NULL, 'i'},
        {"bytes", required_argument, NULL, 'n'},
        {"NoDelay", no_argument, NULL, 'N'},
        {"Set-mss", required_argument, NULL, 'M'},
        {"version", no_argument, NULL, 'v'},
        {"verbose", no_argument, NULL, 'V'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"daemon", no_argument, NULL, 'D'},
        {"format", required_argument, NULL, 'f'},
        {"reverse", no_argument, NULL, 'R'},
        {"version6", no_argument, NULL, '6'},

    /*  XXX: The following ifdef needs to be split up. linux-congestion is not necessarily supported
     *  by systems that support tos.
     */
#ifdef ADD_WHEN_SUPPORTED
        {"tos",        required_argument, NULL, 'S'},
        {"linux-congestion", required_argument, NULL, 'Z'},
#endif
        {NULL, 0, NULL, 0}
    };
    char ch;

    while ((ch = getopt_long(argc, argv, "c:p:st:uP:B:b:l:w:i:n:RS:Nvh6VdM:f:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'c':
                if (test->role == 's') {
                    i_errno = IESERVCLIENT;
                    return (-1);
                } else {
                    test->role = 'c';
                    test->server_hostname = (char *) malloc(strlen(optarg)+1);
                    strncpy(test->server_hostname, optarg, strlen(optarg)+1);
                }
                break;
            case 'p':
                test->server_port = atoi(optarg);
                break;
            case 's':
                if (test->role == 'c') {
                    i_errno = IESERVCLIENT;
                    return (-1);
                } else {
                    test->role = 's';
                }
                break;
            case 't':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->duration = atoi(optarg);
                if (test->duration > MAX_TIME) {
                    i_errno = IEDURATION;
                    return (-1);
                }
                break;
            case 'u':
                if (test->role == 's') {
                    warning("ignoring client only argument --udp (-u)");
                /* XXX: made a warning
                    i_errno = IECLIENTONLY;
                    return (-1);
                */
                }
                set_protocol(test, Pudp);
                test->settings->blksize = DEFAULT_UDP_BLKSIZE;
                break;
            case 'P':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->num_streams = atoi(optarg);
                if (test->num_streams > MAX_STREAMS) {
                    i_errno = IENUMSTREAMS;
                    return (-1);
                }
                break;
            case 'B':
                test->bind_address = (char *) malloc(strlen(optarg)+1);
                strncpy(test->bind_address, optarg, strlen(optarg)+1);
                break;
            case 'b':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->rate = unit_atof(optarg);
                break;
            case 'l':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->blksize = unit_atoi(optarg);
                if (test->settings->blksize > MAX_BLOCKSIZE) {
                    i_errno = IEBLOCKSIZE;
                    return (-1);
                }
                break;
            case 'w':
                // XXX: This is a socket buffer, not specific to TCP
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->socket_bufsize = unit_atof(optarg);
                if (test->settings->socket_bufsize > MAX_TCP_BUFFER) {
                    i_errno = IEBUFSIZE;
                    return (-1);
                }
                break;
            case 'i':
                /* XXX: could potentially want separate stat collection and reporting intervals,
                   but just set them to be the same for now */
                test->stats_interval = atof(optarg);
                test->reporter_interval = atof(optarg);
                if (test->stats_interval > MAX_INTERVAL) {
                    i_errno = IEINTERVAL;
                    return (-1);
                }
                break;
            case 'n':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->bytes = unit_atoi(optarg);
                break;
            case 'N':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->no_delay = 1;
                break;
            case 'M':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->mss = atoi(optarg);
                if (test->settings->mss > MAX_MSS) {
                    i_errno = IEMSS;
                    return (-1);
                }
                break;
            case 'f':
                test->settings->unit_format = *optarg;
                break;
            case '6':
                test->settings->domain = AF_INET6;
                break;
            case 'V':
                test->verbose = 1;
                break;
            case 'd':
                test->debug = 1;
                break;
            case 'R':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->reverse = 1;
                break;
            case 'S':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                // XXX: Checking for errors in strtol is not portable. Leave as is?
                test->settings->tos = strtol(optarg, NULL, 0);
                break;
            case 'v':
                printf(version);
                exit(0);
            case 'h':
            default:
                usage_long();
                exit(1);
        }
    }
    /* For subsequent calls to getopt */
#ifdef __APPLE__
    optreset = 1;
#endif
    optind = 0;

    if ((test->role != 'c') && (test->role != 's')) {
        i_errno = IENOROLE;
        return (-1);
    }

    return (0);
}

int
all_data_sent(struct iperf_test * test)
{
    if (test->settings->bytes > 0) {
        if (test->bytes_sent >= (test->num_streams * test->settings->bytes)) {
            return (1);
        }
    }

    return (0);
}

int
iperf_send(struct iperf_test *test)
{
    int result;
    iperf_size_t bytes_sent;
    fd_set temp_write_set;
    struct timeval tv;
    struct iperf_stream *sp;

    memcpy(&temp_write_set, &test->write_set, sizeof(fd_set));
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    result = select(test->max_fd + 1, NULL, &temp_write_set, NULL, &tv);
    if (result < 0 && errno != EINTR) {
        i_errno = IESELECT;
        return (-1);
    }
    if (result > 0) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            if (FD_ISSET(sp->socket, &temp_write_set)) {
                if ((bytes_sent = sp->snd(sp)) < 0) {
                    i_errno = IESTREAMWRITE;
                    return (-1);
                }
                test->bytes_sent += bytes_sent;
                FD_CLR(sp->socket, &temp_write_set);
            }
        }
    }

    return (0);
}

int
iperf_recv(struct iperf_test *test)
{
    int result;
    iperf_size_t bytes_sent;
    fd_set temp_read_set;
    struct timeval tv;
    struct iperf_stream *sp;

    memcpy(&temp_read_set, &test->read_set, sizeof(fd_set));
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    result = select(test->max_fd + 1, &temp_read_set, NULL, NULL, &tv);
    if (result < 0) {
        i_errno = IESELECT;
        return (-1);
    }
    if (result > 0) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            if (FD_ISSET(sp->socket, &temp_read_set)) {
                if ((bytes_sent = sp->rcv(sp)) < 0) {
                    i_errno = IESTREAMREAD;
                    return (-1);
                }
                test->bytes_sent += bytes_sent;
                FD_CLR(sp->socket, &temp_read_set);
            }
        }
    }

    return (0);
}

int
iperf_init_test(struct iperf_test *test)
{
    struct iperf_stream *sp;
    time_t sec;
    time_t usec;

    if (test->protocol->init) {
        if (test->protocol->init(test) < 0)
            return (-1);
    }

    /* Set timers */
    if (test->settings->bytes == 0) {
        test->timer = new_timer(test->duration, 0);
        if (test->timer == NULL)
            return (-1);
    } 

    if (test->stats_interval != 0) {
        sec = (time_t) test->stats_interval;
        usec = (test->stats_interval - sec) * SEC_TO_US;
        test->stats_timer = new_timer(sec, usec);
        if (test->stats_timer == NULL)
            return (-1);
    }
    if (test->reporter_interval != 0) {
        sec = (time_t) test->reporter_interval;
        usec = (test->reporter_interval - sec) * SEC_TO_US;
        test->reporter_timer = new_timer(sec, usec);
        if (test->reporter_timer == NULL)
            return (-1);
    }

    /* Set start time */
    SLIST_FOREACH(sp, &test->streams, streams) {
        if (gettimeofday(&sp->result->start_time, NULL) < 0) {
            i_errno = IEINITTEST;
            return (-1);
        }
    }

    if (test->on_test_start)
        test->on_test_start(test);

    return (0);
}

/**
 * iperf_exchange_parameters - handles the param_Exchange part for client
 *
 */

int
iperf_exchange_parameters(struct iperf_test * test)
{
    int s, msg;
    char state;

    if (test->role == 'c') {

        if (send_parameters(test) < 0)
            return (-1);

    } else {

        if (get_parameters(test) < 0)
            return (-1);

        if ((s = test->protocol->listen(test)) < 0) {
            state = SERVER_ERROR;
            if (Nwrite(test->ctrl_sck, &state, sizeof(state), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            msg = htonl(i_errno);
            if (Nwrite(test->ctrl_sck, &msg, sizeof(msg), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return (-1);
            }
            msg = htonl(errno);
            if (Nwrite(test->ctrl_sck, &msg, sizeof(msg), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return (-1);
            }
            return (-1);
        }
        FD_SET(s, &test->read_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->prot_listener = s;

        // Send the control message to create streams and start the test
        test->state = CREATE_STREAMS;
        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }

    }

    return (0);
}

/*************************************************************/

int
iperf_exchange_results(struct iperf_test *test)
{
    if (test->role == 'c') {
        /* Send results to server. */
	if (send_results(test) < 0)
            return -1;
        /* Get server results. */
        if (get_results(test) < 0)
            return -1;
    } else {
        /* Get client results. */
        if (get_results(test) < 0)
            return -1;
        /* Send results to client. */
	if (send_results(test) < 0)
            return -1;
    }
    return 0;
}

/*************************************************************/

static int
send_parameters(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;

    j = cJSON_CreateObject();
    if (j == NULL) {
	i_errno = IESENDPARAMS;
	r = -1;
    } else {
	if (test->protocol->id == Ptcp)
	    cJSON_AddTrueToObject(j, "tcp");
	else if (test->protocol->id == Pudp)
	    cJSON_AddTrueToObject(j, "udp");
	if (test->duration)
	    cJSON_AddIntToObject(j, "time", test->duration);
	if (test->settings->bytes)
	    cJSON_AddIntToObject(j, "num", test->settings->bytes);
	if (test->settings->mss)
	    cJSON_AddIntToObject(j, "MSS", test->settings->mss);
	if (test->no_delay)
	    cJSON_AddTrueToObject(j, "nodelay");
	cJSON_AddIntToObject(j, "parallel", test->num_streams);
	if (test->reverse)
	    cJSON_AddTrueToObject(j, "reverse");
	if (test->settings->socket_bufsize)
	    cJSON_AddIntToObject(j, "window", test->settings->socket_bufsize);
	if (test->settings->blksize)
	    cJSON_AddIntToObject(j, "len", test->settings->blksize);
	if (test->settings->rate)
	    cJSON_AddIntToObject(j, "bandwidth", test->settings->rate);
	if (test->settings->tos)
	    cJSON_AddIntToObject(j, "TOS", test->settings->tos);
	if (JSON_write(test->ctrl_sck, j) < 0) {
	    i_errno = IESENDPARAMS;
	    r = -1;
	}
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
get_parameters(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_p;

    j = JSON_read(test->ctrl_sck);
    if (j == NULL) {
	i_errno = IERECVPARAMS;
        r = -1;
    } else {
	if ((j_p = cJSON_GetObjectItem(j, "tcp")) != NULL)
	    set_protocol(test, Ptcp);
	if ((j_p = cJSON_GetObjectItem(j, "udp")) != NULL)
	    set_protocol(test, Pudp);
	if ((j_p = cJSON_GetObjectItem(j, "time")) != NULL)
	    test->duration = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "num")) != NULL)
	    test->settings->bytes = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "MSS")) != NULL)
	    test->settings->mss = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "nodelay")) != NULL)
	    test->no_delay = 1;
	if ((j_p = cJSON_GetObjectItem(j, "parallel")) != NULL)
	    test->num_streams = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "reverse")) != NULL)
	    test->reverse = 1;
	if ((j_p = cJSON_GetObjectItem(j, "window")) != NULL)
	    test->settings->socket_bufsize = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "len")) != NULL)
	    test->settings->blksize = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "bandwidth")) != NULL)
	    test->settings->rate = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "TOS")) != NULL)
	    test->settings->tos = j_p->valueint;
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
send_results(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_streams;
    struct iperf_stream *sp;
    cJSON *j_stream;
    iperf_size_t bytes_transferred;

    j = cJSON_CreateObject();
    if (j == NULL) {
	i_errno = IEPACKAGERESULTS;
	r = -1;
    } else {
	cJSON_AddFloatToObject(j, "cpu_util", test->cpu_util);
	j_streams = cJSON_CreateArray();
	if (j_streams == NULL) {
	    i_errno = IEPACKAGERESULTS;
	    r = -1;
	} else {
	    cJSON_AddItemToObject(j, "streams", j_streams);
	    SLIST_FOREACH(sp, &test->streams, streams) {
		j_stream = cJSON_CreateObject();
		if (j_stream == NULL) {
		    i_errno = IEPACKAGERESULTS;
		    r = -1;
		} else {
		    cJSON_AddItemToArray(j_streams, j_stream);
		    if (test->role == 'c')
			bytes_transferred = (test->reverse ? sp->result->bytes_received : sp->result->bytes_sent);
		    else
			bytes_transferred = (test->reverse ? sp->result->bytes_sent : sp->result->bytes_received);
		    cJSON_AddIntToObject(j_stream, "id", sp->id);
		    cJSON_AddIntToObject(j_stream, "bytes", bytes_transferred);
		    cJSON_AddFloatToObject(j_stream, "jitter", sp->jitter);
		    cJSON_AddIntToObject(j_stream, "errors", sp->cnt_error);
		    cJSON_AddIntToObject(j_stream, "packets", sp->packet_count);
		}
	    }
	    if (r == 0 && JSON_write(test->ctrl_sck, j) < 0) {
		i_errno = IESENDRESULTS;
		r = -1;
	    }
	}
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
get_results(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_cpu_util;
    cJSON *j_streams;
    int n, i;
    cJSON *j_stream;
    cJSON *j_id;
    cJSON *j_bytes;
    cJSON *j_jitter;
    cJSON *j_errors;
    cJSON *j_packets;
    int sid, cerror, pcount;
    double jitter;
    iperf_size_t bytes_transferred;
    struct iperf_stream *sp;

    j = JSON_read(test->ctrl_sck);
    if (j == NULL) {
	i_errno = IERECVRESULTS;
        r = -1;
    } else {
	j_cpu_util = cJSON_GetObjectItem(j, "cpu_util");
	if (j_cpu_util == NULL) {
	    i_errno = IERECVRESULTS;
	    r = -1;
	} else {
	    test->remote_cpu_util = j_cpu_util->valuefloat;
	    j_streams = cJSON_GetObjectItem(j, "streams");
	    if (j_streams == NULL) {
		i_errno = IERECVRESULTS;
		r = -1;
	    } else {
	        n = cJSON_GetArraySize(j_streams);
		for (i=0; i<n; ++i) {
		    j_stream = cJSON_GetArrayItem(j_streams, i);
		    if (j_stream == NULL) {
			i_errno = IERECVRESULTS;
			r = -1;
		    } else {
			j_id = cJSON_GetObjectItem(j_stream, "id");
			j_bytes = cJSON_GetObjectItem(j_stream, "bytes");
			j_jitter = cJSON_GetObjectItem(j_stream, "jitter");
			j_errors = cJSON_GetObjectItem(j_stream, "errors");
			j_packets = cJSON_GetObjectItem(j_stream, "packets");
			if (j_id == NULL || j_bytes == NULL || j_jitter == NULL || j_errors == NULL || j_packets == NULL) {
			    i_errno = IERECVRESULTS;
			    r = -1;
			} else {
			    sid = j_id->valueint;
			    bytes_transferred = j_bytes->valueint;
			    jitter = j_jitter->valuefloat;
			    cerror = j_errors->valueint;
			    pcount = j_packets->valueint;
			    SLIST_FOREACH(sp, &test->streams, streams)
				if (sp->id == sid) break;
			    if (sp == NULL) {
				i_errno = IESTREAMID;
				r = -1;
			    } else {
				if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse)) {
				    sp->jitter = jitter;
				    sp->cnt_error = cerror;
				    sp->packet_count = pcount;
				    sp->result->bytes_received = bytes_transferred;
				} else
				    sp->result->bytes_sent = bytes_transferred;
			    }
			}
		    }
		}
	    }
	}
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
JSON_write(int fd, cJSON *json)
{
    unsigned int hsize, nsize;
    char *str;
    int r = 0;

    str = cJSON_PrintUnformatted(json);
    if (str == NULL)
	r = -1;
    else {
	hsize = strlen(str);
	nsize = htonl(hsize);
	if (Nwrite(fd, &nsize, sizeof(nsize), Ptcp) < 0)
	    r = -1;
	else {
	    if (Nwrite(fd, str, hsize, Ptcp) < 0)
		r = -1;
	}
	free(str);
    }
    return r;
}

/*************************************************************/

static cJSON *
JSON_read(int fd)
{
    unsigned int hsize, nsize;
    char *str;
    cJSON *json = NULL;

    if (Nread(fd, &nsize, sizeof(nsize), Ptcp) >= 0) {
	hsize = ntohl(nsize);
	str = (char *) malloc((hsize+1) * sizeof(char));	/* +1 for EOS */
	if (str != NULL) {
	    if (Nread(fd, str, hsize, Ptcp) >= 0) {
		str[hsize] = '\0';	/* add the EOS */
		json = cJSON_Parse(str);
	    }
	}
	free(str);
    }
    return json;
}

/*************************************************************/
/**
 * add_to_interval_list -- adds new interval to the interval_list
 * XXX: Interval lists should use SLIST implementation fro queue
 */

void
add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results * new)
{
    struct iperf_interval_results *irp = NULL;

    irp = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));
    memcpy(irp, new, sizeof(struct iperf_interval_results));
    TAILQ_INSERT_TAIL(&rp->interval_results, irp, irlistentries);
}


/************************************************************/

/**
 * connect_msg -- displays connection message
 * denoting sender/receiver details
 *
 */

void
connect_msg(struct iperf_stream *sp)
{
    char ipl[INET6_ADDRSTRLEN], ipr[INET6_ADDRSTRLEN];
    int lport, rport, domain = sp->settings->domain;

    if (domain == AF_INET) {
        inet_ntop(domain, (void *) &((struct sockaddr_in *) &sp->local_addr)->sin_addr, ipl, sizeof(ipl));
        inet_ntop(domain, (void *) &((struct sockaddr_in *) &sp->remote_addr)->sin_addr, ipr, sizeof(ipr));
        lport = ntohs(((struct sockaddr_in *) &sp->local_addr)->sin_port);
        rport = ntohs(((struct sockaddr_in *) &sp->remote_addr)->sin_port);
    } else {
        inet_ntop(domain, (void *) &((struct sockaddr_in6 *) &sp->local_addr)->sin6_addr, ipl, sizeof(ipl));
        inet_ntop(domain, (void *) &((struct sockaddr_in6 *) &sp->remote_addr)->sin6_addr, ipr, sizeof(ipr));
        lport = ntohs(((struct sockaddr_in6 *) &sp->local_addr)->sin6_port);
        rport = ntohs(((struct sockaddr_in6 *) &sp->remote_addr)->sin6_port);
    }

    printf("[%3d] local %s port %d connected to %s port %d\n",
        sp->socket, ipl, lport, ipr, rport);
}


/**************************************************************************/

struct iperf_test *
iperf_new_test()
{
    struct iperf_test *test;

    test = (struct iperf_test *) malloc(sizeof(struct iperf_test));
    if (!test) {
        i_errno = IENEWTEST;
        return (NULL);
    }
    /* initialize everything to zero */
    memset(test, 0, sizeof(struct iperf_test));

    test->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memset(test->settings, 0, sizeof(struct iperf_settings));

    return (test);
}

/**************************************************************************/
int
iperf_defaults(struct iperf_test * testp)
{
    testp->duration = DURATION;
    testp->server_port = PORT;
    testp->ctrl_sck = -1;
    testp->prot_listener = -1;

    testp->stats_callback = iperf_stats_callback;
    testp->reporter_callback = iperf_reporter_callback;

    testp->stats_interval = 0;
    testp->reporter_interval = 0;
    testp->num_streams = 1;

    testp->settings->domain = AF_INET;
    testp->settings->unit_format = 'a';
    testp->settings->socket_bufsize = 0;    /* use autotuning */
    testp->settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->settings->rate = RATE;    /* UDP only */
    testp->settings->mss = 0;
    testp->settings->bytes = 0;
    memset(testp->cookie, 0, COOKIE_SIZE);

    /* Set up protocol list */
    SLIST_INIT(&testp->streams);
    SLIST_INIT(&testp->protocols);

    struct protocol *tcp, *udp;
    tcp = (struct protocol *) malloc(sizeof(struct protocol));
    if (!tcp)
        return (-1);
    memset(tcp, 0, sizeof(struct protocol));
    udp = (struct protocol *) malloc(sizeof(struct protocol));
    if (!udp)
        return (-1);
    memset(udp, 0, sizeof(struct protocol));

    tcp->id = Ptcp;
    tcp->name = "TCP";
    tcp->accept = iperf_tcp_accept;
    tcp->listen = iperf_tcp_listen;
    tcp->connect = iperf_tcp_connect;
    tcp->send = iperf_tcp_send;
    tcp->recv = iperf_tcp_recv;
    tcp->init = NULL;
    SLIST_INSERT_HEAD(&testp->protocols, tcp, protocols);

    udp->id = Pudp;
    udp->name = "UDP";
    udp->accept = iperf_udp_accept;
    udp->listen = iperf_udp_listen;
    udp->connect = iperf_udp_connect;
    udp->send = iperf_udp_send;
    udp->recv = iperf_udp_recv;
    udp->init = iperf_udp_init;
    SLIST_INSERT_AFTER(tcp, udp, protocols);

    set_protocol(testp, Ptcp);

    testp->on_new_stream = iperf_on_new_stream;
    testp->on_test_start = iperf_on_test_start;
    testp->on_connect = iperf_on_connect;
    testp->on_test_finish = iperf_on_test_finish;

    return (0);
}


/**************************************************************************/
void
iperf_free_test(struct iperf_test * test)
{
    struct protocol *prot;
    struct iperf_stream *sp;

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }

    free(test->server_hostname);
    free(test->bind_address);
    free(test->settings);
    free_timer(test->timer);
    free_timer(test->stats_timer);
    free_timer(test->reporter_timer);

    /* Free protocol list */
    while (!SLIST_EMPTY(&test->protocols)) {
        prot = SLIST_FIRST(&test->protocols);
        SLIST_REMOVE_HEAD(&test->protocols, protocols);        
        free(prot);
    }

    /* XXX: Why are we setting these values to NULL? */
    // test->streams = NULL;
    test->stats_callback = NULL;
    test->reporter_callback = NULL;
    free(test);
}


void
iperf_reset_test(struct iperf_test *test)
{
    struct iperf_stream *sp;

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }
    free_timer(test->timer);
    free_timer(test->stats_timer);
    free_timer(test->reporter_timer);
    test->timer = NULL;
    test->stats_timer = NULL;
    test->reporter_timer = NULL;

    SLIST_INIT(&test->streams);

    test->role = 's';
    set_protocol(test, Ptcp);
    test->duration = DURATION;
    test->state = 0;
    test->server_hostname = NULL;

    test->ctrl_sck = -1;
    test->prot_listener = -1;

    test->bytes_sent = 0;

    test->reverse = 0;
    test->no_delay = 0;

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = RATE;   /* UDP only */
    test->settings->mss = 0;
    memset(test->cookie, 0, COOKIE_SIZE);
}


/**************************************************************************/

/**
 * iperf_stats_callback -- handles the statistic gathering for both the client and server
 *
 * XXX: This function needs to be updated to reflect the new code
 */


void
iperf_stats_callback(struct iperf_test * test)
{
    struct iperf_stream *sp;
    struct iperf_stream_result *rp = NULL;
    struct iperf_interval_results *irp = NULL, temp;

    SLIST_FOREACH(sp, &test->streams, streams) {
        rp = sp->result;

        if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse))
            temp.bytes_transferred = rp->bytes_sent_this_interval;
        else
            temp.bytes_transferred = rp->bytes_received_this_interval;
     
        irp = TAILQ_FIRST(&rp->interval_results);
        /* result->end_time contains timestamp of previous interval */
        if ( irp != NULL ) /* not the 1st interval */
            memcpy(&temp.interval_start_time, &rp->end_time, sizeof(struct timeval));
        else /* or use timestamp from beginning */
            memcpy(&temp.interval_start_time, &rp->start_time, sizeof(struct timeval));
        /* now save time of end of this interval */
        gettimeofday(&rp->end_time, NULL);
        memcpy(&temp.interval_end_time, &rp->end_time, sizeof(struct timeval));
        temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        //temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        if (has_tcpinfo())
            save_tcpinfo(sp, &temp);
        add_to_interval_list(rp, &temp);
        rp->bytes_sent_this_interval = rp->bytes_received_this_interval = 0;
    }
}

static void
iperf_print_intermediate(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    struct iperf_interval_results *irp;
    iperf_size_t bytes = 0;
    long retransmits = 0;
    double start_time, end_time;

    SLIST_FOREACH(sp, &test->streams, streams) {
        print_interval_results(test, sp);
	/* sum up all streams */
	irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);
        bytes += irp->bytes_transferred;
	retransmits += get_tcpinfo_retransmits(irp);
    }
    if (bytes <=0 ) { /* this can happen if timer goes off just when client exits */
        fprintf(stderr, "error: bytes <= 0!\n");
        return;
    }
    /* next build string with sum of all streams */
    if (test->num_streams > 1) {
        sp = SLIST_FIRST(&test->streams); /* reset back to 1st stream */
        irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);    /* use 1st stream for timing info */

        unit_snprintf(ubuf, UNIT_LEN, (double) (bytes), 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) (bytes / irp->interval_duration),
            test->settings->unit_format);

        start_time = timeval_diff(&sp->result->start_time,&irp->interval_start_time);
        end_time = timeval_diff(&sp->result->start_time,&irp->interval_end_time);
	if (has_tcpinfo_retransmits())
	    printf(report_sum_bw_retrans_format, start_time, end_time, ubuf, nbuf, retransmits);
	else
	    printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
    }
}

static void
iperf_print_results (struct iperf_test *test)
{

    long retransmits = 0, total_retransmits = 0;
    int total_packets = 0, lost_packets = 0;
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes_sent, total_sent = 0;
    iperf_size_t bytes_received, total_received = 0;
    double start_time, end_time, avg_jitter;

    /* print final summary for all intervals */

    if (test->protocol->id == Ptcp)
	if (has_tcpinfo_retransmits())
	    printf(report_bw_retrans_header);
	else
	    printf(report_bw_header);
    else
	printf(report_bw_udp_header);

    start_time = 0.;
    sp = SLIST_FIRST(&test->streams);
    end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
    avg_jitter = 0;
    SLIST_FOREACH(sp, &test->streams, streams) {
        bytes_sent = sp->result->bytes_sent;
        bytes_received = sp->result->bytes_received;
        total_sent += bytes_sent;
        total_received += bytes_received;

        if (test->protocol->id == Ptcp) {
	    if (has_tcpinfo_retransmits()) {
		retransmits = get_tcpinfo_retransmits(TAILQ_LAST(&sp->result->interval_results, irlisthead));
		total_retransmits += retransmits;
	    }
	} else {
            total_packets += sp->packet_count;
            lost_packets += sp->cnt_error;
            avg_jitter += sp->jitter;
        }

        if (bytes_sent > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) (bytes_sent), 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (bytes_sent / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Sent\n");
		if (has_tcpinfo_retransmits())
		    printf(report_bw_retrans_format, sp->socket, start_time, end_time, ubuf, nbuf, retransmits);
		else
		    printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            } else {
                printf(report_bw_udp_format, sp->socket, start_time,
                        end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, 
                        sp->packet_count, (double) (100.0 * sp->cnt_error / sp->packet_count));
                if (test->role == 'c') {
                    printf(report_datagrams, sp->socket, sp->packet_count);
                }
                if (sp->outoforder_packets > 0)
                    printf(report_sum_outoforder, start_time, end_time, sp->cnt_error);
            }
        }
        if (bytes_received > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) bytes_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (bytes_received / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Received\n");
		printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            }
        }
    }

    if (test->num_streams > 1) {
        unit_snprintf(ubuf, UNIT_LEN, (double) total_sent, 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) total_sent / end_time, test->settings->unit_format);
        if (test->protocol->id == Ptcp) {
            printf("      Total sent\n");
	    if (has_tcpinfo_retransmits())
		printf(report_sum_bw_retrans_format, start_time, end_time, ubuf, nbuf, total_retransmits);
	    else
		printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
            unit_snprintf(ubuf, UNIT_LEN, (double) total_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (total_received / end_time), test->settings->unit_format);
            printf("      Total received\n");
	    printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
        } else {
            avg_jitter /= test->num_streams;
            printf(report_sum_bw_udp_format, start_time, end_time, ubuf, nbuf, avg_jitter,
                lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
        }
    }

    if (test->verbose) {
        printf("Host CPU Utilization:   %.1f%%\n", test->cpu_util);
        printf("Remote CPU Utilization: %.1f%%\n", test->remote_cpu_util);
    }
}

/**************************************************************************/

/**
 * iperf_reporter_callback -- handles the report printing
 *
 */

void
iperf_reporter_callback(struct iperf_test * test)
{
    switch (test->state) {
        case TEST_RUNNING:
        case STREAM_RUNNING:
            /* print interval results for each stream */
            iperf_print_intermediate(test);
            break;
        case DISPLAY_RESULTS:
            iperf_print_intermediate(test);
            iperf_print_results(test);
            break;
    } 

}

/**************************************************************************/
void
print_interval_results(struct iperf_test * test, struct iperf_stream * sp)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    double st = 0., et = 0.;
    struct iperf_interval_results *irp = NULL;

    irp = TAILQ_LAST(&sp->result->interval_results, irlisthead); /* get last entry in linked list */
    if (irp == NULL) {
        printf("print_interval_results Error: interval_results = NULL \n");
        return;
    }
    /* First stream? */
    if (sp == SLIST_FIRST(&test->streams)) {
	/* It it's the first interval, print the header;
	** else if there's more than one stream, print the separator;
	** else nothing.
	*/
	if (timeval_equals(&sp->result->start_time, &irp->interval_start_time))
	    if (has_tcpinfo_retransmits())
		printf(report_bw_retrans_header);
	    else
		printf(report_bw_header);
	else if (test->num_streams > 1)
	    printf(report_bw_separator);
    }

    unit_snprintf(ubuf, UNIT_LEN, (double) (irp->bytes_transferred), 'A');
    unit_snprintf(nbuf, UNIT_LEN, (double) (irp->bytes_transferred / irp->interval_duration),
            test->settings->unit_format);
    
    st = timeval_diff(&sp->result->start_time,&irp->interval_start_time);
    et = timeval_diff(&sp->result->start_time,&irp->interval_end_time);
    
    if (has_tcpinfo_retransmits())
	printf(report_bw_retrans_format, sp->socket, st, et, ubuf, nbuf, get_tcpinfo_retransmits(irp));
    else
	printf(report_bw_format, sp->socket, st, et, ubuf, nbuf);
}

/**************************************************************************/
void
iperf_free_stream(struct iperf_stream * sp)
{
    struct iperf_interval_results *irp, *nirp;

    /* XXX: need to free interval list too! */
    free(sp->buffer);
    for (irp = TAILQ_FIRST(&sp->result->interval_results); irp != TAILQ_END(sp->result->interval_results); irp = nirp) {
        nirp = TAILQ_NEXT(irp, irlistentries);
        free(irp);
    }
    free(sp->result);
    free(sp->send_timer);
    free(sp);
}

/**************************************************************************/
struct iperf_stream *
iperf_new_stream(struct iperf_test *test, int s)
{
    int i;
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if (!sp) {
        i_errno = IECREATESTREAM;
        return (NULL);
    }

    memset(sp, 0, sizeof(struct iperf_stream));

    sp->buffer = (char *) malloc(test->settings->blksize);
    sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));
    sp->settings = test->settings;

    if (!sp->buffer) {
        i_errno = IECREATESTREAM;
        return (NULL);
    }
    if (!sp->result) {
        i_errno = IECREATESTREAM;
        return (NULL);
    }

    memset(sp->result, 0, sizeof(struct iperf_stream_result));
    TAILQ_INIT(&sp->result->interval_results);
    
    /* Randomize the buffer */
    srandom(time(NULL));
    for (i = 0; i < test->settings->blksize; ++i)
        sp->buffer[i] = random();

    /* Set socket */
    sp->socket = s;

    sp->snd = test->protocol->send;
    sp->rcv = test->protocol->recv;

    /* Initialize stream */
    if (iperf_init_stream(sp, test) < 0)
        return (NULL);
    iperf_add_stream(test, sp);

    return (sp);
}

/**************************************************************************/
int
iperf_init_stream(struct iperf_stream *sp, struct iperf_test *test)
{
    socklen_t len;
    int opt;

    len = sizeof(struct sockaddr_storage);
    if (getsockname(sp->socket, (struct sockaddr *) &sp->local_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return (-1);
    }
    len = sizeof(struct sockaddr_storage);
    if (getpeername(sp->socket, (struct sockaddr *) &sp->remote_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return (-1);
    }
    /* Set IP TOS */
    if ((opt = test->settings->tos)) {
        if (test->settings->domain == AF_INET6) {
#ifdef IPV6_TCLASS
            if (setsockopt(sp->socket, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETCOS;
                return (-1);
            }
#else
            i_errno = IESETCOS;
            return (-1);
#endif
        } else {
            if (setsockopt(sp->socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETTOS;
                return (-1);
            }
        }
    }

    return (0);
}

/**************************************************************************/
void
iperf_add_stream(struct iperf_test * test, struct iperf_stream * sp)
{
    int i;
    struct iperf_stream *n, *prev;

    if (SLIST_EMPTY(&test->streams)) {
        SLIST_INSERT_HEAD(&test->streams, sp, streams);
        sp->id = 1;
    } else {
        // for (n = test->streams, i = 2; n->next; n = n->next, ++i);
        i = 2;
        SLIST_FOREACH(n, &test->streams, streams) {
            prev = n;
            ++i;
        }
        SLIST_INSERT_AFTER(prev, sp, streams);
        sp->id = i;
    }
}

void
sig_handler(int sig)
{
   longjmp(env, 1); 
}
