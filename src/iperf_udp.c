
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
iperf_udp_recv(struct iperf_stream * sp)
{
    int       result, message;
    int       size = sp->settings->blksize;
    double    transit = 0, d = 0;
    struct udp_datagram *udp = (struct udp_datagram *) sp->buffer;
    struct timeval arrival_time;

    printf("in iperf_udp_recv: reading %d bytes \n", size);
    if (!sp->buffer)
    {
	fprintf(stderr, "receive buffer not allocated \n");
	exit(0);
    }
#ifdef USE_SEND
    do
    {
	result = recv(sp->socket, sp->buffer, size, 0);

    } while (result == -1 && errno == EINTR);
#else
    result = Nread(sp->socket, sp->buffer, size, Pudp);
#endif

    /* interprete the type of message in packet */
    if (result > 0)
    {
	message = udp->state;
    }
    if (message != 7)
    {
	//printf("result = %d state = %d, %d = error\n", result, sp->buffer[0], errno);
    }
    if (message == STREAM_RUNNING && (sp->stream_id == udp->stream_id))
    {
	sp->result->bytes_received += result;
	if (udp->packet_count == sp->packet_count + 1)
	    sp->packet_count++;

	/* jitter measurement */
	if (gettimeofday(&arrival_time, NULL) < 0)
	{
	    perror("gettimeofday");
	}
	transit = timeval_diff(&udp->sent_time, &arrival_time);
	d = transit - sp->prev_transit;
	if (d < 0)
	    d = -d;
	sp->prev_transit = transit;
	sp->jitter += (d - sp->jitter) / 16.0;


	/* OUT OF ORDER PACKETS */
	if (udp->packet_count != sp->packet_count)
	{
	    if (udp->packet_count < sp->packet_count + 1)
	    {
		sp->outoforder_packets++;
		printf("OUT OF ORDER - incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);
	    } else
		sp->cnt_error += udp->packet_count - sp->packet_count;
	}
	/* store the latest packet id */
	if (udp->packet_count > sp->packet_count)
	    sp->packet_count = udp->packet_count;

	//printf("incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);

    }
    return message;

}


/**************************************************************************/
int
iperf_udp_send(struct iperf_stream * sp)
{
    int       result = 0;
    struct timeval before, after;
    int64_t   dtargus;
    int64_t   adjustus = 0;

    //printf("in iperf_udp_send \n");
    /*
     * the || part ensures that last packet is sent to server - the
     * STREAM_END MESSAGE
     */
    if (sp->send_timer->expired(sp->send_timer) || sp->settings->state == STREAM_END)
    {
	int       size = sp->settings->blksize;

	/* this is for udp packet/jitter/lost packet measurements */
	struct udp_datagram *udp = (struct udp_datagram *) sp->buffer;
	struct param_exchange *param = NULL;

	dtargus = (int64_t) (sp->settings->blksize) * SEC_TO_US * 8;
	dtargus /= sp->settings->rate;

	assert(dtargus != 0);

	switch (sp->settings->state)
	{
	case STREAM_BEGIN:
	    udp->state = STREAM_BEGIN;
	    udp->stream_id = (int) sp;
	    /* udp->packet_count = ++sp->packet_count; */
	    break;

	case STREAM_END:
	    udp->state = STREAM_END;
	    udp->stream_id = (int) sp;
	    break;

	case RESULT_REQUEST:
	    udp->state = RESULT_REQUEST;
	    udp->stream_id = (int) sp;
	    break;

	case ALL_STREAMS_END:
	    udp->state = ALL_STREAMS_END;
	    break;

	case STREAM_RUNNING:
	    udp->state = STREAM_RUNNING;
	    udp->stream_id = (int) sp;
	    udp->packet_count = ++sp->packet_count;
	    break;
	}

	if (sp->settings->state == STREAM_BEGIN)
	{
	    sp->settings->state = STREAM_RUNNING;
	}
	if (gettimeofday(&before, 0) < 0)
	    perror("gettimeofday");

	udp->sent_time = before;

	printf("iperf_udp_send: writing %d bytes \n", size);
#ifdef USE_SEND
	result = send(sp->socket, sp->buffer, size, 0);
#else
	result = Nwrite(sp->socket, sp->buffer, size, Pudp);
#endif

	if (gettimeofday(&after, 0) < 0)
	    perror("gettimeofday");

	/*
	 * CHECK: Packet length and ID if(sp->settings->state ==
	 * STREAM_RUNNING) printf("State = %d Outgoing packet = %d AND SP =
	 * %d\n",sp->settings->state, sp->packet_count, sp->socket);
	 */

	if (sp->settings->state == STREAM_RUNNING)
	    sp->result->bytes_sent += result;

	adjustus = dtargus;
	adjustus += (before.tv_sec - after.tv_sec) * SEC_TO_US;
	adjustus += (before.tv_usec - after.tv_usec);

	if (adjustus > 0)
	{
	    dtargus = adjustus;
	}
	/* RESET THE TIMER  */
	update_timer(sp->send_timer, 0, dtargus);
	param = NULL;

    }				/* timer_expired_micro */
    return result;
}

/**************************************************************************/
struct iperf_stream *
iperf_new_udp_stream(struct iperf_test * testp)
{
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) iperf_new_stream(testp);
    if (!sp)
    {
	perror("malloc");
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
iperf_udp_accept(struct iperf_test * test)
{

    struct iperf_stream *sp;
    struct sockaddr_in sa_peer;
    char     *buf;
    socklen_t len;
    int       sz;

    buf = (char *) malloc(test->default_settings->blksize);
    struct udp_datagram *udp = (struct udp_datagram *) buf;

    len = sizeof sa_peer;

    sz = recvfrom(test->listener_sock_udp, buf, test->default_settings->blksize, 0, (struct sockaddr *) & sa_peer, &len);

    if (!sz)
	return -1;

    if (connect(test->listener_sock_udp, (struct sockaddr *) & sa_peer, len) < 0)
    {
	perror("connect");
	return -1;
    }
    sp = test->new_stream(test);

    sp->socket = test->listener_sock_udp;

    setnonblocking(sp->socket);

    iperf_init_stream(sp, test);
    iperf_add_stream(test, sp);


    test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
    if (test->listener_sock_udp < 0)
	return -1;

    FD_SET(test->listener_sock_udp, &test->read_set);
    test->max_fd = (test->max_fd < test->listener_sock_udp) ? test->listener_sock_udp : test->max_fd;

    if (test->default_settings->state != RESULT_REQUEST)
	connect_msg(sp);

    printf("iperf_udp_accept: 1st UDP data  packet for socket %d has arrived \n", sp->socket);
    sp->stream_id = udp->stream_id;
    sp->result->bytes_received += sz;

    /* Count OUT OF ORDER PACKETS */
    if (udp->packet_count != 0)
    {
	if (udp->packet_count < sp->packet_count + 1)
	    sp->outoforder_packets++;
	else
	    sp->cnt_error += udp->packet_count - sp->packet_count;
    }
    /* store the latest packet id */
    if (udp->packet_count > sp->packet_count)
	sp->packet_count = udp->packet_count;

    //printf("incoming packet = %d and received packet = %d AND SP = %d\n", udp->packet_count, sp->packet_count, sp->socket);

    free(buf);
    return 0;
}

