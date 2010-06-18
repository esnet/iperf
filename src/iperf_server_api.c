
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

void handle_message(struct iperf_test * test, int m, struct iperf_stream * sp);

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
    char     *buf = (char *) calloc(1, sizeof(struct param_exchange));

    if (sp->settings->cookie[0] == '\0' ||
	(strncmp(param->cookie, sp->settings->cookie, COOKIE_SIZE) == 0))
    {
	strncpy(sp->settings->cookie, param->cookie, COOKIE_SIZE);
	sp->settings->blksize = param->blksize;
	sp->settings->socket_bufsize = param->recv_window;
	sp->settings->unit_format = param->format;
	printf("Got params from client: block size = %d, recv_window = %d cookie = %s\n",
	       sp->settings->blksize, sp->settings->socket_bufsize, sp->settings->cookie);
	param->state = TEST_START;
	buf[0] = TEST_START;

    } else
    {
	fprintf(stderr, "Connection from new client denied.\n");
	printf("cookie still set to %s \n", sp->settings->cookie);
	param->state = ACCESS_DENIED;
	buf[0] = ACCESS_DENIED;
    }
    memcpy(sp->buffer, buf, sizeof(struct param_exchange));;
    //printf("param_received: Sending message (%d) back to client \n", sp->buffer[0]);
    result = Nwrite(sp->socket, sp->buffer, sizeof(struct param_exchange), Ptcp);
    if (result < 0)
	perror("param_received: Error sending param ack to client");
    free(buf);
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

int
iperf_server_listen(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    int x;

/*    
    if (test->protocol == Pudp) {
        test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
        if (test->listener_sock_udp < 0) {
            // Needs to set some sort of error number/message
            return -1;
        }
    }
 
    test->listener_sock_tcp = netannounce(Ptcp, NULL, test->server_port);
    if (test->listener_sock_tcp < 0) {
        // Needs to set some sort of error number/message
        return -1;
    }

    if (test->protocol == Ptcp) {
        if (set_tcp_windowsize(test->listener_sock_tcp, test->default_settings->socket_bufsize, SO_RCVBUF) < 0) {
            // Needs to set some sort of error number/message
            perror("unable to set TCP window");
            return -1;
        }
    }

    // make sure that accept call does not block
    setnonblocking(test->listener_sock_tcp);
    setnonblocking(test->listener_sock_udp);
*/
    if((test->listener = netannounce(Ptcp, NULL, test->server_port)) < 0) {
        // Needs to set some sort of error number/message
        return -1;
    }
    setnonblocking(test->listener);

    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", test->server_port);

    // This needs to be changed to reflect if client has different window size
    // make sure we got what we asked for
    if ((x = get_tcp_windowsize(test->listener_sock_tcp, SO_RCVBUF)) < 0) {
        // Needs to set some sort of error number/message
        perror("SO_RCVBUF");
        return -1;
    }

    // This code needs to be moved to after parameter exhange
    if (test->protocol == Ptcp) {
        if (test->default_settings->socket_bufsize > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
            printf("TCP window size: %s\n", ubuf);
        } else {
            printf("Using TCP Autotuning\n");
        }
    }
    printf("-----------------------------------------------------------\n");

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->temp_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = test->ctrl_sck;

    return 0;
}

int
iperf_accept(struct iperf_test *test)
{
    int s;
    char ipl[512], ipr[512];
    socklen_t len;
    struct sockaddr_in addr;

    if (test->ctrl_sck == 0) {
        if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
            perror("accept");
            return -1;
        }

        inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) & addr.local_addr)->sin_addr),
                (void *) ipl, sizeof(ipl));
        inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) & addr.remote_addr)->sin_addr),
                (void *) ipr, sizeof(ipr));
        printf(report_peer, s,
                ipl, ntohs(((struct sockaddr_in *) & addr.local_addr)->sin_port),
                ipr, ntohs(((struct sockaddr_in *) & addr.remote_addr)->sin_port));

        return s;
    } else {
        // This message needs to be sent to the client
        printf("The server is busy running a test. Try again later.\n");
        return 0;
    }
}

/**************************************************************************/
int
iperf_handle_message(struct iperf_test *test)
{
    if (read(test->ctrl_sck, &test->state, sizeof(int)) < 0) {
        // indicate error on read
        return -1;
    }

    switch(test->state) {
        case PARAM_EXCHANGE:
            iperf_exchange_parameters(test);
            break;
    }

    return 0;
}

void
iperf_run_server(struct iperf_test *test)
{
    struct timeval tv;
    //struct iperf_stream *np;
    struct iperf_stream *sp;
    struct timer *stats_interval, *reporter_interval;
    char *result_string = NULL;
    int j = 0, result = 0, message = 0;
    int nfd = 0;

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        // This needs to be replaced by more formal error handling
        fprintf(stderr, "An error occurred. Exiting.\n");
        exit(1);
    }

    test->num_streams = 0;
    test->default_settings->state = TEST_RUNNING;

    printf("iperf_run_server: Waiting for client connect.... \n");

    while (test->default_settings != TEST_END) {

        // Copy select set and renew timers
        FD_COPY(&test->read_set, &test->temp_set);
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        result = select(test->max_fd + 1, &test->temp_set, NULL, NULL, &tv);
        if (result < 0 && errno != EINTR) {
            // Change the way this handles errors
            perror("select");
            exit(1);
        } else if (result > 0) {
            if (FD_ISSET(test->listener, &test->temp_set)) {
                test->ctrl_sck = iperf_accept(test);
                if (test->ctrl_sck < 0) {
                    fprintf(stderr, "error: could not open control socket. exiting.\n");
                    exit(1);
                } else if (test->ctrl_sck > 0) {
                    // Accepted! exchange parameters / setup

                }
                FD_CLR(test->listener, &test->temp_set);
            }
            if (FD_ISSET(test->ctrl_sck, &test->temp_set)) {
                // Handle control messages

                FD_CLR(test->ctrl_sck, &test->temp_set);                
            }
            if (FD_ISSET(test->prot_listener, &test->temp_set)) {
                // Spawn new streams

                FD_CLR(test->ctrl_sck, &test->temp_set);
            }
            // Iterate through the streams to see if their socket FD_ISSET
            for (sp = test->streams; sp != NULL; sp = sp->next) {
                if (FD_ISSET(sp->socket, &test->temp_set)) {


                }
            }
        }
    }

/*
    while (test->default_settings->state != TEST_END) {
        memcpy(&test->temp_set, &test->read_set, sizeof(test->read_set));
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        // using select to check on multiple descriptors.
        result = select(test->max_fd + 1, &test->temp_set, NULL, NULL, &tv);
        if (result == 0) {
            continue;
        } else if (result < 0 && errno != EINTR) {
            printf("Error in select(): %s, socket = %d\n", strerror(errno), test->max_fd + 1);
            exit(0);
        } else if (result > 0) {
            if (test->protocol == Ptcp) {
                // Accept a new TCP connection 
                if (FD_ISSET(test->ctrl_sck, &test->temp_set)) {
                    test->protocol = Ptcp;

                    // The following line needs to be moved to test initialization
                    test->accept = iperf_tcp_accept;
                    if (test->accept < 0) // .. really??!
                        return;

                    // Again, needs to be moved to test initialization
                    test->new_stream = iperf_new_tcp_stream;
                    test->accept(test);
                    test->default_settings->state = TEST_RUNNING;
                    FD_CLR(test->listener_sock_tcp, &test->temp_set);
                    //printf("iperf_run_server: accepted TCP connection \n");
                    test->num_streams++;
                }
            } else {
                // Accept a new UDP connection
                if (FD_ISSET(test->listener_sock_udp, &test->temp_set)) {
                    test->protocol = Pudp;
                    test->accept = iperf_udp_accept;
                    if (test->accept < 0)
                        return;
                    test->new_stream = iperf_new_udp_stream;
                    test->accept(test);
                    test->default_settings->state = TEST_RUNNING;
                    FD_CLR(test->listener_sock_udp, &test->temp_set);
                    printf("iperf_run_server: accepted UDP connection \n");
                }
            }
            // Process the sockets for read operation
            nfd = test->max_fd + 1;
            for (j = 0; j <= test->max_fd; j++) {
                if (FD_ISSET(j, &test->temp_set)) {
                    // find the correct stream - possibly time consuming?
                    np = find_stream_by_socket(test, j);
                    message = np->rcv(np);	// get data from client using receiver callback
                    // This code needs to be fixed to work without goto
                    if (message < 0)
                        goto done;
                    handle_message(test, message, np);
                    if (message == TEST_END)
                        break;
                }
            }

            if (message == PARAM_EXCHANGE) {
                // start timer at end of PARAM_EXCHANGE
                if (test->stats_interval != 0)
                    stats_interval = new_timer(test->stats_interval, 0);
                if (test->reporter_interval != 0)
                    reporter_interval = new_timer(test->reporter_interval, 0);
            }

            if ((message == STREAM_BEGIN) || (message == STREAM_RUNNING)) {
                //
                // XXX: is this right? Might there be cases where we want
                // stats for while in another state?
                //
                if ((test->stats_interval != 0) && stats_interval->expired(stats_interval)) {
                    test->stats_callback(test);
                    update_timer(stats_interval, test->stats_interval, 0);
                }
                if ((test->reporter_interval != 0) && reporter_interval->expired(reporter_interval)) {
                    test->reporter_callback(test);
                    update_timer(reporter_interval, test->reporter_interval, 0);
                }
            }
        }
    }
*/ // End of the WHILE

done:
    printf("Test Complete. \n\n");

    /* reset cookie when client is finished */
    /* XXX: which cookie to reset, and why is it stored to 2 places? */
    //memset(test->streams->settings->cookie, '\0', COOKIE_SIZE);
    memset(test->default_settings->cookie, '\0', COOKIE_SIZE);
    return;
}

/********************************************************/

void
handle_message(struct iperf_test * test, int message, struct iperf_stream * sp)
{
    char     *results_string = NULL;
    struct iperf_stream *tp1 = NULL;
    struct iperf_stream *tp2 = NULL;

    //printf("handle_message: %d \n", message);
    if (message < 0)
    {
	printf("Error reading data from client \n");
	close(sp->socket);
	return;
    }
    sp->settings->state = message;

    if (message == PARAM_EXCHANGE)
    {
	/* copy the received settings into test */
	memcpy(test->default_settings, test->streams->settings, sizeof(struct iperf_settings));
    }
    if (message == ACCESS_DENIED)	/* this might get set by
					 * PARAM_EXCHANGE */
    {
	/* XXX: test this! */
	close(sp->socket);
	FD_CLR(sp->socket, &test->read_set);
	iperf_free_stream(sp);
    }
    if (message == STREAM_END)
    {
	/* get final timestamp for all streams */
	tp1 = test->streams;
	do
	{
	    gettimeofday(&tp1->result->end_time, NULL);
	    tp1 = tp1->next;
	} while (tp1 != NULL);
    }
    if (message == RESULT_REQUEST)
    {
	sp->settings->state = RESULT_RESPOND;
        test->stats_callback(test);
	    test->reporter_callback(test);
    // results_string = test->reporter_callback(test);
	// sp->data = results_string;
	// send_result_to_client(sp);
    }
    if (message == ALL_STREAMS_END)
    {
	printf("Client done sending data. Printing final results. \n");
	/* print server results */
        test->stats_callback(test);  
        test->reporter_callback(test);
    }
    if (message == TEST_END)
    {
	//printf("client sent TEST_END message, shuting down sockets.. \n");
	/* FREE ALL STREAMS  */
	tp1 = test->streams;
	do
	{
	    tp2 = tp1;
	    //printf(" closing socket: %d \n", tp2->socket);
	    close(tp2->socket);
	    FD_CLR(tp2->socket, &test->read_set);
	    tp1 = tp2->next;	/* get next pointer before freeing */
	    iperf_free_stream(tp2);
	} while (tp1 != NULL);

	test->default_settings->state = TEST_END;
	memset(test->default_settings->cookie, '\0', COOKIE_SIZE);
    }
}

/**************************************************************************/

/**
 * find_stream_by_socket -- finds the stream based on socket ID
 *   simple sequential scan: not more effiecient, but should be fine
 *	for a small number of streams.
 *
 *returns stream
 *
 */

struct iperf_stream *
find_stream_by_socket(struct iperf_test * test, int sock)
{
    struct iperf_stream *n;

    n = test->streams;
    while (1)
    {
	if (n->socket == sock)
	    break;
	if (n->next == NULL)
	    break;

	n = n->next;
    }
    return n;
}
