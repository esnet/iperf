
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
#include "iperf_server_api.h"
#include "iperf_tcp.h"
#include "timer.h"
#include "net.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

jmp_buf   env;			/* to handle longjmp on signal */

/**************************************************************************/

/**
 * iperf_tcp_recv -- receives the data for TCP
 * and the Param/result message exchange
 *returns state of packet received
 *
 */

int
iperf_tcp_recv(struct iperf_stream * sp)
{
    int       result = 0, message = 0;
    int       size = sp->settings->blksize;
    char     *final_message = NULL;

    errno = 0;

    struct param_exchange *param = (struct param_exchange *) sp->buffer;

    if (!sp->buffer)
    {
	fprintf(stderr, "receive buffer not allocated \n");
	return -1;
    }
    /* get the 1st byte: then based on that, decide how much to read */
    if ((result = recv(sp->socket, &message, sizeof(int), MSG_PEEK)) != sizeof(int))
    {
	if (result == 0)
	    printf("Client Disconnected. \n");
	else
	    perror("iperf_tcp_recv: recv error: MSG_PEEK");
	return -1;
    }
    sp->settings->state = message;

#ifdef DEBUG
    if (message != STREAM_RUNNING)	/* tell me about non STREAM_RUNNING messages
				 * for debugging */
	printf("iperf_tcp_recv: got message type %d \n", message);
#endif

    switch (message)
    {
    case PARAM_EXCHANGE:
	size = sizeof(struct param_exchange);
#ifdef USE_RECV
	do
	{
	    result = recv(sp->socket, sp->buffer, size, MSG_WAITALL);
	} while (result == -1 && errno == EINTR);
#else
	result = Nread(sp->socket, sp->buffer, size, Ptcp);
#endif
	if (result == -1)
	{
	    perror("iperf_tcp_recv: recv error");
	    return -1;
	}
	//printf("iperf_tcp_recv: recv returned %d bytes \n", result);
	//printf("result = %d state = %d, %d = error\n", result, sp->buffer[0], errno);
	result = param_received(sp, param);	/* handle PARAM_EXCHANGE and
						 * send result to client */

	break;

    case TEST_START:
    case STREAM_BEGIN:
    case STREAM_RUNNING:
	size = sp->settings->blksize;
#ifdef USE_RECV
	/*
	 * NOTE: Nwrite/Nread seems to be 10-15% faster than send/recv for
	 * localhost on OSX. More testing needed on other OSes to be sure.
	 */
	do
	{
	    //printf("iperf_tcp_recv: Calling recv: expecting %d bytes \n", size);
	    result = recv(sp->socket, sp->buffer, size, MSG_WAITALL);

	} while (result == -1 && errno == EINTR);
#else
	result = Nread(sp->socket, sp->buffer, size, Ptcp);
#endif
	if (result == -1)
	{
	    perror("Read error");
	    return -1;
	}
	//printf("iperf_tcp_recv: recv on socket %d returned %d bytes \n", sp->socket, result);
	sp->result->bytes_received += result;
	break;
    case STREAM_END:
	size = sizeof(struct param_exchange);
	result = Nread(sp->socket, sp->buffer, size, Ptcp);
	break;
    case ALL_STREAMS_END:
	size = sizeof(struct param_exchange);
	result = Nread(sp->socket, sp->buffer, size, Ptcp);
	break;
    case TEST_END:
	size = sizeof(struct param_exchange);
	result = Nread(sp->socket, sp->buffer, size, Ptcp);
	break;
    case RESULT_REQUEST:
	/* XXX: not working yet  */
	//final_message = iperf_reporter_callback(test);
	//memcpy(sp->buffer, final_message, strlen(final_message));
	//result = send(sp->socket, sp->buffer, MAX_RESULT_STRING, 0);
	if (result < 0)
	    perror("Error sending results back to client");

	break;
    default:
	printf("unexpected state encountered: %d \n", message);
	return -1;
    }

    return message;
}

/**************************************************************************/

/**
 * iperf_tcp_send -- sends the client data for TCP
 * and the  Param/result message exchanges
 * returns: bytes sent
 *
 */
int
iperf_tcp_send(struct iperf_stream * sp)
{
    int       result;
    int       size = sp->settings->blksize;

    if (!sp->buffer)
    {
	perror("transmit buffer not allocated");
	return -1;
    }

    //printf("iperf_tcp_send: state = %d \n", sp->settings->state);
    memcpy(sp->buffer, &(sp->settings->state), sizeof(int));;

    /* set read size based on message type */
    switch (sp->settings->state)
    {
    case PARAM_EXCHANGE:
	size = sizeof(struct param_exchange);
	break;
    case STREAM_BEGIN:
	size = sp->settings->blksize;
	break;
    case STREAM_END:
	size = sizeof(struct param_exchange);
	break;
    case RESULT_REQUEST:
	size = MAX_RESULT_STRING;
	break;
    case ALL_STREAMS_END:
	size = sizeof(struct param_exchange);
	break;
    case TEST_END:
	size = sizeof(struct param_exchange);
	break;
    case STREAM_RUNNING:
	size = sp->settings->blksize;
	break;
    default:
	printf("State of the stream can't be determined\n");
	return -1;
    }

    //if(sp->settings->state != STREAM_RUNNING)
    //    printf("   in iperf_tcp_send, message type = %d (total = %d bytes) \n", sp->settings->state, size);

#ifdef USE_SEND
    result = send(sp->socket, sp->buffer, size, 0);
#else
    result = Nwrite(sp->socket, sp->buffer, size, Ptcp);
#endif
    if (result < 0)
	perror("Write error");
    //printf("   iperf_tcp_send: %d bytes sent \n", result);

    if (sp->settings->state == STREAM_BEGIN || sp->settings->state == STREAM_RUNNING)
	sp->result->bytes_sent += result;

    //printf("iperf_tcp_send: number bytes sent so far = %u \n", (uint64_t) sp->result->bytes_sent);

    /* change state after 1st send */
    if (sp->settings->state == STREAM_BEGIN)
	sp->settings->state = STREAM_RUNNING;

    return result;
}

/**************************************************************************/
struct iperf_stream *
iperf_new_tcp_stream(struct iperf_test * testp)
{
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) iperf_new_stream(testp);
    if (!sp)
    {
	perror("malloc");
	return (NULL);
    }
    sp->rcv = iperf_tcp_recv;	/* pointer to receive function */
    sp->snd = iperf_tcp_send;	/* pointer to send function */

    /* XXX: not yet written...  (what is this supposed to do? ) */
    //sp->update_stats = iperf_tcp_update_stats;

    return sp;
}

/**************************************************************************/

/**
 * iperf_tcp_accept -- accepts a new TCP connection
 * on tcp_listener_socket for TCP data and param/result
 * exchange messages
 * returns 0 on success
 *
 */

int
iperf_tcp_accept(struct iperf_test * test)
{
    socklen_t len;
    struct sockaddr_in addr;
    int       peersock;
    struct iperf_stream *sp;

    len = sizeof(addr);
    peersock = accept(test->listener_sock_tcp, (struct sockaddr *) & addr, &len);
    if (peersock < 0)
    {
	printf("Error in accept(): %s\n", strerror(errno));
	return -1;
    } else
    {
	sp = test->new_stream(test);
	setnonblocking(peersock);

	FD_SET(peersock, &test->read_set);  /* add new socket to master set */
	test->max_fd = (test->max_fd < peersock) ? peersock : test->max_fd;
        //printf("iperf_tcp_accept: max_fd now set to: %d \n", test->max_fd );

	sp->socket = peersock;
	//printf("in iperf_tcp_accept: socket = %d, tcp_windowsize: %d \n", peersock, test->default_settings->socket_bufsize);
	iperf_init_stream(sp, test);
	iperf_add_stream(test, sp);

	if (test->default_settings->state != RESULT_REQUEST)
	    connect_msg(sp);	/* print connect message */

	return 0;
    }
}


