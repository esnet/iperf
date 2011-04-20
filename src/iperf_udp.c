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
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/select.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_error.h"
#include "timer.h"
#include "net.h"


/* iperf_udp_recv
 *
 * receives the data for UDP
 */
int
iperf_udp_recv(struct iperf_stream *sp)
{
    int       result;
    int       size = sp->settings->blksize;
    int       sec, usec, pcount;
    double    transit = 0, d = 0;
    struct timeval sent_time, arrival_time;

    result = Nread(sp->socket, sp->buffer, size, Pudp);

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

    return (result);
}


/* iperf_udp_send
 *
 * sends the data for UDP
 */
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

        result = Nwrite(sp->socket, sp->buffer, size, Pudp);

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

    return (result);
}


/**************************************************************************/

/* iperf_udp_accept
 *
 * accepts a new UDP connection
 */
int
iperf_udp_accept(struct iperf_test *test)
{
    struct sockaddr_in sa_peer;
    int       buf;
    socklen_t len;
    int       sz, s;

    s = test->prot_listener;

    len = sizeof sa_peer;
    if ((sz = recvfrom(test->prot_listener, &buf, sizeof(buf), 0, (struct sockaddr *) &sa_peer, &len)) < 0) {
        i_errno = IESTREAMACCEPT;
        return (-1);
    }

    if (connect(s, (struct sockaddr *) &sa_peer, len) < 0) {
        i_errno = IESTREAMACCEPT;
        return (-1);
    }

    test->prot_listener = netannounce(test->settings->domain, Pudp, test->bind_address, test->server_port);
    if (test->prot_listener < 0) {
        i_errno = IESTREAMLISTEN;
        return (-1);
    }

    FD_SET(test->prot_listener, &test->read_set);
    test->max_fd = (test->max_fd < test->prot_listener) ? test->prot_listener : test->max_fd;

    /* Let the client know we're ready "accept" another UDP "stream" */
    if (write(s, &buf, sizeof(buf)) < 0) {
        i_errno = IESTREAMWRITE;
        return (-1);
    }

    return (s);
}


/* iperf_udp_listen
 *
 * start up a listener for UDP stream connections
 */
int
iperf_udp_listen(struct iperf_test *test)
{
    int s;

    if ((s = netannounce(test->settings->domain, Pudp, test->bind_address, test->server_port)) < 0) {
        i_errno = IESTREAMLISTEN;
        return (-1);
    }

    return (s);
}


/* iperf_udp_connect
 *
 * connect to a TCP stream listener
 */
int
iperf_udp_connect(struct iperf_test *test)
{
    int s, buf;

    if ((s = netdial(test->settings->domain, Pudp, test->bind_address, test->server_hostname, test->server_port)) < 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    /* Write to the UDP stream to give the server this stream's credentials */
    if (write(s, &buf, sizeof(buf)) < 0) {
        // XXX: Should this be changed to IESTREAMCONNECT? 
        i_errno = IESTREAMWRITE;
        return (-1);
    }
    /* Wait until the server confirms the client UDP write */
    // XXX: Should this read be TCP instead?
    if (read(s, &buf, sizeof(buf)) < 0) {
        i_errno = IESTREAMREAD;
        return (-1);
    }

    return (s);
}


/* iperf_udp_init
 *
 * initializer for UDP streams in TEST_START
 */
int
iperf_udp_init(struct iperf_test *test)
{
    int64_t dtargus;
    struct iperf_stream *sp;

    /* Calculate the send delay needed to hit target bandwidth (-b) */
    dtargus = (int64_t) test->settings->blksize * SEC_TO_US * 8;
    dtargus /= test->settings->rate;

    assert(dtargus != 0);

//    for (sp = test->streams; sp; sp = sp->next) {
    SLIST_FOREACH(sp, &test->streams, streams) {
        sp->send_timer = new_timer(dtargus / SEC_TO_US, dtargus % SEC_TO_US);
        if (sp->send_timer == NULL)
            return (-1);
    }

    return (0);
}
