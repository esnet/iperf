
/*
 * Copyright (c) 2009, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

/* iperf_udp.c:  UDP specific routines for iperf
 *
 * NOTE: not yet finished / working 
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
#include <signal.h>
#include <setjmp.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_error.h"
#include "timer.h"
#include "net.h"
#include "locale.h"


/**************************************************************************/

/**
 * iperf_udp_recv -- receives the client data for UDP
 *
 *returns state of packet received
 *
 */

int
iperf_udp_recv(struct iperf_stream *sp)
{
    int       result;
    int       size = sp->settings->blksize;
    int       sec, usec, pcount;
    double    transit = 0, d = 0;
    struct timeval sent_time, arrival_time;

#ifdef USE_SEND
    do {
        result = recv(sp->socket, sp->buffer, size, 0);
    } while (result == -1 && errno == EINTR);
#else
    result = Nread(sp->socket, sp->buffer, size, Pudp);
#endif

    if (result < 0) {
        return (-1);
    }
    sp->result->bytes_received += result;
    sp->result->bytes_received_this_interval += result;

    memcpy(&sec, sp->buffer, sizeof(sec));
    memcpy(&usec, sp->buffer+4, sizeof(usec));
    memcpy(&pcount, sp->buffer+8, sizeof(pcount));
    sec = ntohl(sec);
    usec = ntohl(usec);
    pcount = ntohl(pcount);
    sent_time.tv_sec = sec;
    sent_time.tv_usec = usec;

    /* Out of order packets */
    if (pcount >= sp->packet_count + 1) {
        if (pcount > sp->packet_count + 1) {
            sp->cnt_error += (pcount - 1) - sp->packet_count;
        }
        sp->packet_count = pcount;
    } else {
        sp->outoforder_packets++;
        printf("OUT OF ORDER - incoming packet = %d and received packet = %d AND SP = %d\n", pcount, sp->packet_count, sp->socket);
    }

    /* jitter measurement */
    gettimeofday(&arrival_time, NULL);

    transit = timeval_diff(&sent_time, &arrival_time);
    d = transit - sp->prev_transit;
    if (d < 0)
        d = -d;
    sp->prev_transit = transit;
    // XXX: This is NOT the way to calculate jitter
    //      J = |(R1 - S1) - (R0 - S0)| [/ number of packets, for average]
    sp->jitter += (d - sp->jitter) / 16.0;

    return result;
}


/**************************************************************************/
int
iperf_udp_send(struct iperf_stream *sp)
{
    ssize_t   result = 0;
    int64_t   dtargus;
    int64_t   adjustus = 0;
    uint64_t  sec, usec, pcount;
    int       size = sp->settings->blksize;
    struct timeval before, after;

    if (timer_expired(sp->send_timer)) {

        dtargus = (int64_t) (sp->settings->blksize) * SEC_TO_US * 8;
        dtargus /= sp->settings->rate;

        assert(dtargus != 0);

        gettimeofday(&before, 0);

        ++sp->packet_count;
        sec = htonl(before.tv_sec);
        usec = htonl(before.tv_usec);
        pcount = htonl(sp->packet_count);

        memcpy(sp->buffer, &sec, sizeof(sec));
        memcpy(sp->buffer+4, &usec, sizeof(usec));
        memcpy(sp->buffer+8, &pcount, sizeof(pcount));

#ifdef USE_SEND
        result = send(sp->socket, sp->buffer, size, 0);
#else
        result = Nwrite(sp->socket, sp->buffer, size, Pudp);
#endif

        if (result < 0)
            return (-1);

        sp->result->bytes_sent += result;
        sp->result->bytes_sent_this_interval += result;

        gettimeofday(&after, 0);

        adjustus = dtargus;
        adjustus += (before.tv_sec - after.tv_sec) * SEC_TO_US;
        adjustus += (before.tv_usec - after.tv_usec);

        if (adjustus > 0) {
            dtargus = adjustus;
        }

        if (update_timer(sp->send_timer, 0, dtargus) < 0)
            return (-1);
    }

    return result;
}

/**************************************************************************/
struct iperf_stream *
iperf_new_udp_stream(struct iperf_test * testp)
{
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) iperf_new_stream(testp);
    if (!sp) {
        return (NULL);
    }
    sp->rcv = iperf_udp_recv;
    sp->snd = iperf_udp_send;

    return sp;
}

/**************************************************************************/

/**
 * iperf_udp_accept -- accepts a new UDP connection
 * on udp_listener_socket
 *returns 0 on success
 *
 */


int
iperf_udp_accept(struct iperf_test *test)
{
    struct iperf_stream *sp;
    struct sockaddr_in sa_peer;
    int       buf;
    socklen_t len;
    int       sz, s;

    s = test->listener_udp;

    len = sizeof sa_peer;
    if ((sz = recvfrom(test->listener_udp, &buf, sizeof(buf), 0, (struct sockaddr *) &sa_peer, &len)) < 0) {
        i_errno = IESTREAMACCEPT;
        return (-1);
    }

    if (connect(s, (struct sockaddr *) &sa_peer, len) < 0) {
        i_errno = IESTREAMACCEPT;
        return (-1);
    }

    sp = test->new_stream(test);
    if (!sp)
        return (-1);
    sp->socket = s;
    if (iperf_init_stream(sp, test) < 0)
        return (-1);
    iperf_add_stream(test, sp);
    FD_SET(s, &test->read_set);
    FD_SET(s, &test->write_set);
    test->max_fd = (s > test->max_fd) ? s : test->max_fd;

    test->listener_udp = netannounce(Pudp, NULL, test->server_port);
    if (test->listener_udp < 0) {
        i_errno = IELISTEN;
        return (-1);
    }

    FD_SET(test->listener_udp, &test->read_set);
    test->max_fd = (test->max_fd < test->listener_udp) ? test->listener_udp : test->max_fd;

    /* Let the client know we're ready "accept" another UDP "stream" */
    if (write(sp->socket, &buf, sizeof(buf)) < 0) {
        i_errno = IESTREAMWRITE;
        return (-1);
    }

    connect_msg(sp);
    test->streams_accepted++;

    return (0);
}

