
/*
 * Copyright (c) 2009, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

/* iperf_server_api.c: Functions to be used by an iperf server
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
#include "iperf_server_api.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

/*********************************************************************/
/**
 * param_received - handles the param_Exchange part for server
 * returns state on success, -1 on failure
 *
 */

int
param_received(struct iperf_stream * sp, struct param_exchange * param)
{
    int       result;
    char     *buf = (char *) malloc(sizeof(struct param_exchange));

    if (sp->settings->cookie[0] == '\0' ||
	(strncmp(param->cookie, sp->settings->cookie, 37) == 0))
    {
	strncpy(sp->settings->cookie, param->cookie, 37);
	sp->settings->blksize = param->blksize;
	sp->settings->socket_bufsize = param->recv_window;
	sp->settings->unit_format = param->format;
	printf("Got params from client: block size = %d, recv_window = %d cookie = %s\n",
	       sp->settings->blksize, sp->settings->socket_bufsize, sp->settings->cookie);
	param->state = TEST_START;
	buf[0] = TEST_START;

    } else
    {
	fprintf(stderr, "Connection from new client denied\n");
	param->state = ACCESS_DENIED;
	buf[0] = ACCESS_DENIED;
    }
    free(buf);
    printf("param_received: Sending message back to client \n");
    result = send(sp->socket, buf, sizeof(struct param_exchange), 0);
    if (result < 0)
	perror("param_received: Error sending param ack to client");
    return param->state;
}

/*************************************************************/

/**
 * send_result_to_client - sends result string from server to client
 */

void
send_result_to_client(struct iperf_stream * sp)
{
    int       result;
    int       size = sp->settings->blksize;

    char     *buf = (char *) malloc(size);

    if (!buf)
    {
	perror("malloc: unable to allocate transmit buffer");
    }
    /* adding the string terminator to the message */
    buf[strlen((char *) sp->data)] = '\0';

    memcpy(buf, sp->data, strlen((char *) sp->data));

    printf("send_result_to_client: sending %d bytes \n", (int) strlen((char *) sp->data));
    result = send(sp->socket, buf, size, 0);
    printf("RESULT SENT TO CLIENT\n");

    free(buf);
}

/**************************************************************************/
void
iperf_run_server(struct iperf_test * test)
{
    struct timeval tv;
    struct iperf_stream *np, *sp;
    int       j, result, message;
    char     *results_string = NULL;

    FD_ZERO(&test->read_set);
    FD_SET(test->listener_sock_tcp, &test->read_set);
    FD_SET(test->listener_sock_udp, &test->read_set);

    test->max_fd = test->listener_sock_tcp > test->listener_sock_udp ? test->listener_sock_tcp : test->listener_sock_udp;

    test->num_streams = 0;
    test->default_settings->state = TEST_RUNNING;

    printf("iperf_run_server: Waiting for client connect.... \n");
    while (test->default_settings->state != TEST_END)
    {
	memcpy(&test->temp_set, &test->read_set, sizeof(test->read_set));
	tv.tv_sec = 15;
	tv.tv_usec = 0;

	/* using select to check on multiple descriptors. */
	result = select(test->max_fd + 1, &test->temp_set, NULL, NULL, &tv);

	if (result == 0)
	{
	    //printf("SERVER IDLE : %d sec\n", (int) tv.tv_sec);
	    continue;
	} else if (result < 0 && errno != EINTR)
	{
	    printf("Error in select(): %s\n", strerror(errno));
	    exit(0);
	} else if (result > 0)
	{
	    /* Accept a new TCP connection */
	    if (FD_ISSET(test->listener_sock_tcp, &test->temp_set))
	    {
		test->protocol = Ptcp;
		test->accept = iperf_tcp_accept;
		test->new_stream = iperf_new_tcp_stream;
		test->accept(test);
		test->default_settings->state = TEST_RUNNING;
		FD_CLR(test->listener_sock_tcp, &test->temp_set);
	    }
	    /* Accept a new UDP connection */
	    else if (FD_ISSET(test->listener_sock_udp, &test->temp_set))
	    {
		test->protocol = Pudp;
		test->accept = iperf_udp_accept;
		test->new_stream = iperf_new_udp_stream;
		test->accept(test);
		test->default_settings->state = TEST_RUNNING;
		FD_CLR(test->listener_sock_udp, &test->temp_set);
	    }
	    /* Process the sockets for read operation */
	    for (j = 0; j < test->max_fd + 1; j++)
	    {
		if (FD_ISSET(j, &test->temp_set))
		{
		    /* find the correct stream - possibly time consuming */
		    np = find_stream_by_socket(test, j);
		    message = np->rcv(np);	/* get data from client using receiver callback  */
	            //printf ("iperf_run_server: iperf_tcp_recv returned %d \n", message);
		    np->settings->state = message;

		    if (message == PARAM_EXCHANGE)
		    {
			/* copy the received settings into test */
			memcpy(test->default_settings, test->streams->settings, sizeof(struct iperf_settings));
		    }
		    if (message == ACCESS_DENIED)  /* this might get set by PARAM_EXCHANGE */
		    {
		        /* XXX: test this! */
			close(np->socket);
			FD_CLR(np->socket, &test->read_set);
			iperf_free_stream(test, np);
                    }
		    if (message == STREAM_END)
		    {
			/*
			 * XXX: should I expect one of these per stream. If
			 * so, only timestamp the 1st one??
			 */
			gettimeofday(&np->result->end_time, NULL);
		    }
		    if (message == RESULT_REQUEST)
		    {
			np->settings->state = RESULT_RESPOND;
			results_string = test->reporter_callback(test);
			np->data = results_string;
			send_result_to_client(np);
		    }
		    if (message == ALL_STREAMS_END)
		    {
			/* print server results */
			results_string = test->reporter_callback(test);
			puts(results_string);	/* send to stdio */
		    }
		    if (message == TEST_END)
		    {
			/* FREE ALL STREAMS  */
			np = test->streams;
			do
			{
			    sp = np;
			    close(sp->socket);
			    FD_CLR(sp->socket, &test->read_set);
			    np = sp->next;
			    iperf_free_stream(test, sp);
			} while (np != NULL);

			printf("TEST_END\n\n");
			test->default_settings->state = TEST_END;
			/* reset cookie with client is finished */
			memset(test->default_settings->cookie, '\0', 37);

		    }
		}		/* end if (FD_ISSET(j, &temp_set)) */
	    }			/* end for (j=0;...) */

	}			/* end else (result>0)   */
    }				/* end while */

}

