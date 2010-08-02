
/*
 * Copyright (c) 2009, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
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
#include <sys/queue.h>
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
#include "iperf_error.h"
#include "timer.h"

#include "units.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "locale.h"

jmp_buf env;			/* to handle longjmp on signal */

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
    int domain;

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
        {"tcpInfo", no_argument, NULL, 'T'},
        {"bandwidth", required_argument, NULL, 'b'},
        {"length", required_argument, NULL, 'l'},
        {"window", required_argument, NULL, 'w'},
        {"interval", required_argument, NULL, 'i'},
        {"bytes", required_argument, NULL, 'n'},
        {"NoDelay", no_argument, NULL, 'N'},
        {"Print-mss", no_argument, NULL, 'm'},
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

    while ((ch = getopt_long(argc, argv, "c:p:st:uP:B:b:l:w:i:n:mRNTvh6VdM:f:", longopts, NULL)) != -1) {
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
                    i_errno = IECLIENTONLY;
                    return (-1);
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
            case 'm':
                test->print_mss = 1;
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
            case 'T':
                test->tcp_info = 1;
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


/*********************************************************/

int
package_parameters(struct iperf_test *test)
{
    char pstring[256];
    char optbuf[128];
    memset(pstring, 0, 256*sizeof(char));

    *pstring = ' ';

    if (test->protocol->id == Ptcp) {
        strncat(pstring, "-p ", sizeof(pstring));
    } else if (test->protocol->id == Pudp) {
        strncat(pstring, "-u ", sizeof(pstring));
    }

    snprintf(optbuf, sizeof(optbuf), "-P %d ", test->num_streams);
    strncat(pstring, optbuf, sizeof(pstring));
    
    if (test->reverse)
        strncat(pstring, "-R ", sizeof(pstring));
    
    if (test->settings->socket_bufsize) {
        snprintf(optbuf, sizeof(optbuf), "-w %d ", test->settings->socket_bufsize);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->settings->rate) {
        snprintf(optbuf, sizeof(optbuf), "-b %llu ", test->settings->rate);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->settings->mss) {
        snprintf(optbuf, sizeof(optbuf), "-m %d ", test->settings->mss);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->no_delay) {
        snprintf(optbuf, sizeof(optbuf), "-N ");
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->settings->bytes) {
        snprintf(optbuf, sizeof(optbuf), "-n %llu ", test->settings->bytes);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->duration) {
        snprintf(optbuf, sizeof(optbuf), "-t %d ", test->duration);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    if (test->settings->blksize) {
        snprintf(optbuf, sizeof(optbuf), "-l %d ", test->settings->blksize);
        strncat(pstring, optbuf, sizeof(pstring));
    }

    *pstring = (char) (strlen(pstring) - 1);

    if (Nwrite(test->ctrl_sck, pstring, strlen(pstring), Ptcp) < 0) {
        i_errno = IESENDPARAMS;
        return (-1);
    }

    return 0;
}


int
parse_parameters(struct iperf_test *test)
{
    int n;
    char *param, **params;
    char len, ch;
    char pstring[256];

    memset(pstring, 0, 256 * sizeof(char));

    if (Nread(test->ctrl_sck, &len, sizeof(char), Ptcp) < 0) {
        i_errno = IERECVPARAMS;
        return (-1);
    }

    if (Nread(test->ctrl_sck, pstring, len, Ptcp) < 0) {
        i_errno = IERECVPARAMS;
        return (-1);
    }

    for (param = strtok(pstring, " "), n = 0, params = NULL; param; param = strtok(NULL, " ")) {
        if ((params = realloc(params, (n+1)*sizeof(char *))) == NULL) {
            i_errno = IERECVPARAMS;
            return (-1);
        }
        params[n] = param;
        n++;
    }

    // XXX: Should we check for parameters exceeding maximum values here?
    while ((ch = getopt(n, params, "pt:n:m:uNP:Rw:l:b:")) != -1) {
        switch (ch) {
            case 'p':
                set_protocol(test, Ptcp);
                break;
            case 't':
                test->duration = atoi(optarg);
                break;
            case 'n':
                test->settings->bytes = atoll(optarg);
                break;
            case 'm':
                test->settings->mss = atoi(optarg);
                break;
            case 'u':
                set_protocol(test, Pudp);
                break;
            case 'N':
                test->no_delay = 1;
                break;
            case 'P':
                test->num_streams = atoi(optarg);
                break;
            case 'R':
                test->reverse = 1;
                break;
            case 'w':
                test->settings->socket_bufsize = atoi(optarg);
                break;
            case 'l':
                test->settings->blksize = atoi(optarg);
                break;
            case 'b':
                test->settings->rate = atoll(optarg);
                break;
        }
    }
#ifdef __APPLE__
    optreset = 1;
#endif
    optind = 0;

    free(params);

    return (0);
}

/**
 * iperf_exchange_parameters - handles the param_Exchange part for client
 *
 */

int
iperf_exchange_parameters(struct iperf_test * test)
{
    int s;

    if (test->role == 'c') {

        if (package_parameters(test) < 0)
            return (-1);

    } else {

        if (parse_parameters(test) < 0)
            return (-1);

        if ((s = test->protocol->listen(test)) < 0)
            return (-1);
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
    unsigned int size;
    char buf[128];
    char *results;
    struct iperf_stream *sp;
    iperf_size_t bytes_transferred;

    if (test->role == 'c') {
        /* Prepare results string and send to server */
        results = NULL;
        size = 0;
//        for (sp = test->streams; sp; sp = sp->next) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            bytes_transferred = (test->reverse ? sp->result->bytes_received : sp->result->bytes_sent);
            snprintf(buf, 128, "%d:%llu,%lf,%d,%d\n", sp->id, bytes_transferred,sp->jitter,
                sp->cnt_error, sp->packet_count);
            size += strlen(buf);
            if ((results = realloc(results, size+1)) == NULL) {
                i_errno = IEPACKAGERESULTS;
                return (-1);
            }
            if (sp == SLIST_FIRST(&test->streams))
                *results = '\0';
            strncat(results, buf, size+1);
        }
        size++;
        size = htonl(size);
        if (Nwrite(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        if (Nwrite(test->ctrl_sck, results, ntohl(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        free(results);

        /* Get server results string */
        if (Nread(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        size = ntohl(size);
        results = (char *) malloc(size * sizeof(char));
        if (results == NULL) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        if (Nread(test->ctrl_sck, results, size, Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }

        // XXX: The only error this sets is IESTREAMID, which may never be reached. Consider making void.
        if (parse_results(test, results) < 0)
            return (-1);

        free(results);

    } else {
        /* Get client results string */
        if (Nread(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        size = ntohl(size);
        results = (char *) malloc(size * sizeof(char));
        if (results == NULL) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        if (Nread(test->ctrl_sck, results, size, Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }

        // XXX: Same issue as with client
        if (parse_results(test, results) < 0)
            return (-1);

        free(results);

        /* Prepare results string and send to client */
        results = NULL;
        size = 0;
//        for (sp = test->streams; sp; sp = sp->next) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            bytes_transferred = (test->reverse ? sp->result->bytes_sent : sp->result->bytes_received);
            snprintf(buf, 128, "%d:%llu,%lf,%d,%d\n", sp->id, bytes_transferred, sp->jitter,
                sp->cnt_error, sp->packet_count);
            size += strlen(buf);
            if ((results = realloc(results, size+1)) == NULL) {
                i_errno = IEPACKAGERESULTS;
                return (-1);
            }
            if (sp == SLIST_FIRST(&test->streams))
                *results = '\0';
            strncat(results, buf, size+1);
        }
        size++;
        size = htonl(size);
        if (Nwrite(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        if (Nwrite(test->ctrl_sck, results, ntohl(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        free(results);

    }

    return (0);
}

/*************************************************************/

int
parse_results(struct iperf_test *test, char *results)
{
    int sid, cerror, pcount;
    double jitter;
    char *strp;
    iperf_size_t bytes_transferred;
    struct iperf_stream *sp;

    for (strp = results; *strp; strp = strchr(strp, '\n')+1) {
        sscanf(strp, "%d:%llu,%lf,%d,%d\n", &sid, &bytes_transferred, &jitter,
            &cerror, &pcount);
//        for (sp = test->streams; sp; sp = sp->next)
        SLIST_FOREACH(sp, &test->streams, streams)
            if (sp->id == sid) break;
        if (sp == NULL) {
            i_errno = IESTREAMID;
            return (-1);
        }
        if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse)) {
            sp->jitter = jitter;
            sp->cnt_error = cerror;
            sp->packet_count = pcount;
            sp->result->bytes_received = bytes_transferred;
        } else
            sp->result->bytes_sent = bytes_transferred;
    }

    return (0);
}


/*************************************************************/
/**
 * add_to_interval_list -- adds new interval to the interval_list
 *
 */

void
add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results * new)
{
    struct iperf_interval_results *ip = NULL;

    ip = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));
    memcpy(ip, new, sizeof(struct iperf_interval_results));
    ip->next = NULL;

    if (rp->interval_results == NULL) {	/* if 1st interval */
        rp->interval_results = ip;
        rp->last_interval_results = ip; /* pointer to last element in list */
    } else { /* add to end of list */
        rp->last_interval_results->next = ip;
        rp->last_interval_results = ip;
    }
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
    testp->settings->socket_bufsize = 0;	/* use autotuning */
    testp->settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->settings->rate = RATE;	/* UDP only */
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
    struct iperf_interval_results *ip = NULL, temp;

    SLIST_FOREACH(sp, &test->streams, streams) {
        rp = sp->result;

        if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse))
            temp.bytes_transferred = rp->bytes_sent_this_interval;
        else
            temp.bytes_transferred = rp->bytes_received_this_interval;
     
        ip = sp->result->interval_results;
        /* result->end_time contains timestamp of previous interval */
        if ( ip != NULL ) /* not the 1st interval */
            memcpy(&temp.interval_start_time, &sp->result->end_time, sizeof(struct timeval));
        else /* or use timestamp from beginning */
            memcpy(&temp.interval_start_time, &sp->result->start_time, sizeof(struct timeval));
        /* now save time of end of this interval */
        gettimeofday(&sp->result->end_time, NULL);
        memcpy(&temp.interval_end_time, &sp->result->end_time, sizeof(struct timeval));
        temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        //temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        if (test->tcp_info)
            get_tcpinfo(test, &temp);
        add_to_interval_list(rp, &temp);
        rp->bytes_sent_this_interval = rp->bytes_received_this_interval = 0;

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
    int total_packets = 0, lost_packets = 0;
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes = 0, bytes_sent = 0, bytes_received = 0;
    iperf_size_t total_sent = 0, total_received = 0;
    double start_time, end_time, avg_jitter;
    struct iperf_interval_results *ip = NULL;

    switch (test->state) {
        case TEST_RUNNING:
        case STREAM_RUNNING:
            /* print interval results for each stream */
//            for (sp = test->streams; sp != NULL; sp = sp->next) {
            SLIST_FOREACH(sp, &test->streams, streams) {
                print_interval_results(test, sp);
                bytes += sp->result->interval_results->bytes_transferred; /* sum up all streams */
            }
            if (bytes <=0 ) { /* this can happen if timer goes off just when client exits */
                fprintf(stderr, "error: bytes <= 0!\n");
                break;
            }
            /* next build string with sum of all streams */
            if (test->num_streams > 1) {
                sp = SLIST_FIRST(&test->streams); /* reset back to 1st stream */
                ip = sp->result->last_interval_results;	/* use 1st stream for timing info */

                unit_snprintf(ubuf, UNIT_LEN, (double) (bytes), 'A');
                unit_snprintf(nbuf, UNIT_LEN, (double) (bytes / ip->interval_duration),
                        test->settings->unit_format);

                start_time = timeval_diff(&sp->result->start_time,&ip->interval_start_time);
                end_time = timeval_diff(&sp->result->start_time,&ip->interval_end_time);
                printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);

#if defined(linux) || defined(__FreeBSD__)			/* is it usful to figure out a way so sum * TCP_info acrross multiple streams? */
                if (test->tcp_info)
                    print_tcpinfo(ip);
#endif
            }
            break;
        case DISPLAY_RESULTS:
            /* print final summary for all intervals */

            printf(report_bw_header);

            start_time = 0.;
            sp = SLIST_FIRST(&test->streams);
            end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
//            for (sp = test->streams; sp != NULL; sp = sp->next) {
            SLIST_FOREACH(sp, &test->streams, streams) {
                bytes_sent = sp->result->bytes_sent;
                bytes_received = sp->result->bytes_received;
                total_sent += bytes_sent;
                total_received += bytes_received;

                if (test->protocol->id == Pudp) {
                    total_packets += sp->packet_count;
                    lost_packets += sp->cnt_error;
                    avg_jitter += sp->jitter;
                }

                if (bytes_sent > 0) {
                    unit_snprintf(ubuf, UNIT_LEN, (double) (bytes_sent), 'A');
                    unit_snprintf(nbuf, UNIT_LEN, (double) (bytes_sent / end_time), test->settings->unit_format);
                    if (test->protocol->id == Ptcp) {
                        printf("      Sent\n");
                        printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);

#if defined(linux) || defined(__FreeBSD__)
                        if (test->tcp_info) {
                            ip = sp->result->last_interval_results;	
                            print_tcpinfo(ip);
                        }
#endif
                    } else {
                        printf(report_bw_jitter_loss_format, sp->socket, start_time,
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
                    printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
                    unit_snprintf(ubuf, UNIT_LEN, (double) total_received, 'A');
                    unit_snprintf(nbuf, UNIT_LEN, (double) (total_received / end_time), test->settings->unit_format);
                    printf("      Total received\n");
                    printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
                } else {
                    avg_jitter /= test->num_streams;
                    printf(report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf, avg_jitter,
                        lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
                }

                // XXX: Why is this here?
                if ((test->print_mss != 0) && (test->role == 'c')) {
                    printf("The TCP maximum segment size mss = %d\n", getsock_tcp_mss(sp->socket));
                }
            }
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
    struct iperf_interval_results *ir = NULL;

    ir = sp->result->last_interval_results; /* get last entry in linked list */
    if (ir == NULL) {
        printf("print_interval_results Error: interval_results = NULL \n");
        return;
    }
    if (sp == SLIST_FIRST(&test->streams)) {
        printf(report_bw_header);
    }

    unit_snprintf(ubuf, UNIT_LEN, (double) (ir->bytes_transferred), 'A');
    unit_snprintf(nbuf, UNIT_LEN, (double) (ir->bytes_transferred / ir->interval_duration),
            test->settings->unit_format);
    
    st = timeval_diff(&sp->result->start_time,&ir->interval_start_time);
    et = timeval_diff(&sp->result->start_time,&ir->interval_end_time);
    
    printf(report_bw_format, sp->socket, st, et, ubuf, nbuf);

#if defined(linux) || defined(__FreeBSD__)
    if (test->tcp_info)
        print_tcpinfo(ir);
#endif
}

/**************************************************************************/
void
iperf_free_stream(struct iperf_stream * sp)
{
    struct iperf_interval_results *ip, *np;

    /* XXX: need to free interval list too! */
    free(sp->buffer);
    for (ip = sp->result->interval_results; ip; ip = np) {
        np = ip->next;
        free(ip);
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
iperf_init_stream(struct iperf_stream * sp, struct iperf_test * testp)
{
    socklen_t len;

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

