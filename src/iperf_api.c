
/*
 * Copyright (c) 2004, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

/*
 * TO DO list:
 *    test TCP_INFO
 *    restructure code pull out main.c
 *    cleanup/fix/test UDP mode
 *    add verbose and debug options
 *    lots more testing
 *    see issue tracker for other wish list items
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

#include "iperf_api.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

jmp_buf   env; /* to handle longjmp on signal */

/*************************************************************/
int
all_data_sent(struct iperf_test * test)
{
    if (test->default_settings->bytes == 0)
	return 0;
    else
    {
	uint64_t       total_bytes = 0;
	struct iperf_stream *sp;

	sp = test->streams;

	while (sp)
	{
	    total_bytes += sp->result->bytes_sent;
	    sp = sp->next;
	}

	if (total_bytes >= (test->num_streams * test->default_settings->bytes))
	{
	    return 1;
	} else
	    return 0;
    }

}

/*********************************************************/
void
exchange_parameters(struct iperf_test * test)
{
    int       result;
    struct iperf_stream *sp;
    struct param_exchange *param;

    //printf("in exchange_parameters \n");
    sp = test->streams;
    sp->settings->state = PARAM_EXCHANGE;
    param = (struct param_exchange *) sp->buffer;

    get_uuid(test->default_settings->cookie);
    strncpy(param->cookie, test->default_settings->cookie, 37);

    /* setting up exchange parameters  */
    param->state = PARAM_EXCHANGE;
    param->blksize = test->default_settings->blksize;
    param->recv_window = test->default_settings->socket_bufsize;
    param->send_window = test->default_settings->socket_bufsize;
    param->format = test->default_settings->unit_format;

    //printf(" sending exchange params: size = %d \n", (int) sizeof(struct param_exchange));
    /* XXX: cant we use iperf_tcp_send for this? that would be cleaner */
    result = send(sp->socket, sp->buffer, sizeof(struct param_exchange), 0);
    if (result < 0)
	perror("Error sending exchange params to server");

    /* XXX: no error checking! -blt */

    //printf("result = %d state = %d, error = %d \n", result, sp->buffer[0], errno);

    /* get answer back from server */
    do
    {
	//printf("exchange_parameters: reading result from server .. \n");
	result = recv(sp->socket, sp->buffer, sizeof(struct param_exchange), 0);
    } while (result == -1 && errno == EINTR);
    if (result < 0)
	perror("Error getting exchange params ack from server");

    if (result > 0 && sp->buffer[0] == ACCESS_DENIED)
    {
	fprintf(stderr, "Busy server Detected. Exiting.\n");
        exit(-1);
    }
     
    return;
}

/*********************************************************************/
int
param_received(struct iperf_stream * sp, struct param_exchange * param)
{
    int       result;
    char     *buf = (char *) malloc(sizeof(struct param_exchange));

    if (sp->settings->cookie[0] == '\0')
    {
	strncpy(sp->settings->cookie, param->cookie, 37);
	sp->settings->blksize = param->blksize;
	sp->settings->socket_bufsize = param->recv_window;
	sp->settings->unit_format = param->format;
        printf("Got params from client: block size = %d, recv_window = %d \n",
		sp->settings->blksize, sp->settings->socket_bufsize);
        param->state = TEST_START;
	buf[0] = TEST_START;

    } else if (strncmp(param->cookie, sp->settings->cookie, 37) != 0)
    {
	fprintf(stderr, "New connection denied\n");
        param->state = ACCESS_DENIED;
	buf[0] = ACCESS_DENIED;
    }

    free(buf);
    result = send(sp->socket, buf, sizeof(struct param_exchange), 0);
    if (result < 0) 
	perror("Error sending param ack to client");
    return param->state;
}

/*************************************************************/
void
setnonblocking(int sock)
{
    int       opts;

    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
	perror("fcntl(F_SETFL)");
	exit(EXIT_FAILURE);
    }
    return;
}

/*************************************************************/
void
add_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results temp)
{
    struct iperf_interval_results *n;

    struct iperf_interval_results *ip = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));

    ip->bytes_transferred = temp.bytes_transferred;
    ip->interval_duration = temp.interval_duration;
#if defined(linux) || defined(__FreeBSD__)
    ip->tcpInfo = temp.tcpInfo;
#endif
    //printf("add_interval_list: Mbytes = %d, duration = %f \n", (int)(ip->bytes_transferred/1000000), ip->interval_duration);

    if (!rp->interval_results)
    {
	rp->interval_results = ip;
    } else
    {
	n = rp->interval_results;
	while (n->next)
	    n = n->next;

	n->next = ip;
    }
    ip->next = NULL;
}

/*************************************************************/

/* for debugging only */
void
display_interval_list(struct iperf_stream_result * rp, int tflag)
{
    struct iperf_interval_results *n;

    n = rp->interval_results;

    while (n)
    {
	printf("Interval = %f\tBytes transferred = %llu\n", n->interval_duration, n->bytes_transferred);
	if (tflag)
            print_tcpinfo(n);
	n = n->next;
    }
}

/*************************************************************/
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

    result = send(sp->socket, buf, size, 0);
    printf("RESULT SENT TO CLIENT\n");

    free(buf);
}

/************************************************************/
void
receive_result_from_server(struct iperf_test * test)
{
    int       result;
    struct iperf_stream *sp;
    int       size = 0;
    char     *buf = NULL;

    printf("in receive_result_from_server \n");
    size = sp->settings->blksize;	/* XXX: Is blksize really what we
					 * want to use for the results? */

    buf = (char *) malloc(size);
    sp = test->streams;

    printf("receive_result_from_server: send ALL_STREAMS_END to server \n");
    sp->settings->state = ALL_STREAMS_END;
    sp->snd(sp);		/* send message to server */

    printf("receive_result_from_server: send RESULT_REQUEST to server \n");
    sp->settings->state = RESULT_REQUEST;
    sp->snd(sp);		/* send message to server */

    /* receive from server */

    do
    {
	result = recv(sp->socket, buf, size, 0);
    } while (result == -1 && errno == EINTR);
    printf("size of results from server: %d \n", result);

    printf(server_reporting, sp->socket);
    puts(buf);			/* prints results */

}

/*************************************************************************/
int
getsock_tcp_mss(int inSock)
{
    int       mss = 0;

    int       rc;
    socklen_t len;

    assert(inSock >= 0);

    /* query for mss */
    len = sizeof(mss);
    rc = getsockopt(inSock, IPPROTO_TCP, TCP_MAXSEG, (char *) &mss, &len);

    return mss;
}

/*************************************************************/
int
set_socket_options(struct iperf_stream * sp, struct iperf_test * tp)
{

    socklen_t len;

    if (tp->no_delay == 1)
    {
	int       no_delay = 1;

	len = sizeof(no_delay);
	int       rc = setsockopt(sp->socket, IPPROTO_TCP, TCP_NODELAY,
				  (char *) &no_delay, len);

	if (rc == -1)
	{
	    perror("TCP_NODELAY");
	    return -1;
	}
    }
#ifdef TCP_MAXSEG
    if (tp->default_settings->mss > 0)
    {
	int       rc;
	int       new_mss;

	len = sizeof(new_mss);

	assert(sp->socket != -1);

	/* set */
	new_mss = tp->default_settings->mss;
	len = sizeof(new_mss);
	rc = setsockopt(sp->socket, IPPROTO_TCP, TCP_MAXSEG, (char *) &new_mss, len);
	if (rc == -1)
	{
	    perror("setsockopt");
	    return -1;
	}
	/* verify results */
	rc = getsockopt(sp->socket, IPPROTO_TCP, TCP_MAXSEG, (char *) &new_mss, &len);
	if (new_mss != tp->default_settings->mss)
	{
	    perror("setsockopt value mismatch");
	    return -1;
	}
    }
#endif
    return 0;
}

/*************************************************************/
void
connect_msg(struct iperf_stream * sp)
{
    char      ipl[512], ipr[512];

    inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) & sp->local_addr)->sin_addr), (void *) ipl, sizeof(ipl));
    inet_ntop(AF_INET, (void *) (&((struct sockaddr_in *) & sp->remote_addr)->sin_addr), (void *) ipr, sizeof(ipr));

    printf("[%3d] local %s port %d connected to %s port %d\n",
	   sp->socket,
	   ipl, ntohs(((struct sockaddr_in *) & sp->local_addr)->sin_port),
	   ipr, ntohs(((struct sockaddr_in *) & sp->remote_addr)->sin_port));
}

/*************************************************************/
void
Display(struct iperf_test * test)
{
    struct iperf_stream *n;

    n = test->streams;
    int       count = 1;

    printf("===============DISPLAY==================\n");

    while (1)
    {
	if (n)
	{
	    if (test->role == 'c')
		printf("position-%d\tsp=%d\tsocket=%d\tbytes sent=%llu\n", count++, (int) n, n->socket, n->result->bytes_sent);
	    else
		printf("position-%d\tsp=%d\tsocket=%d\tbytes received=%llu\n", count++, (int) n, n->socket, n->result->bytes_received);

	    if (n->next == NULL)
	    {
		printf("=================END====================\n");
		fflush(stdout);
		break;
	    }
	    n = n->next;
	}
    }
}

/**************************************************************************/
int
iperf_tcp_recv(struct iperf_stream * sp)
{
    int       result, message;
    char      ch;
    int       size = sp->settings->blksize;

    errno = 0;

    struct param_exchange *param = (struct param_exchange *) sp->buffer;

    if (!sp->buffer)
    {
	fprintf(stderr, "receive buffer not allocated \n");
	return -1;
    }
    /* get the 1st byte: then based on that, decide how much to read */
    if ((result = recv(sp->socket, &ch, sizeof(int), MSG_PEEK)) != sizeof(int))
    {
	if (result == 0)
	    printf("Client Disconnected. \n");
        else
	    perror("iperf_tcp_recv: recv error: MSG_PEEK");
	return -1;
    }

    message = (int) ch;
    if( message != 7) /* tell me about non STREAM_RUNNING messages for debugging */
        printf("iperf_tcp_recv: got message type %d \n", message);

    switch (message)
    {
    case PARAM_EXCHANGE:
    case RESULT_REQUEST:
    case TEST_START:
	size = sizeof(struct param_exchange);
	do
	{			/* XXX: is the do/while really needed? */
	    //printf("iperf_tcp_recv: Calling recv: expecting %d bytes \n", size);
	    result = recv(sp->socket, sp->buffer, size, MSG_WAITALL);
	} while (result == -1 && errno == EINTR);
	if (result == -1)
	{
	    perror("iperf_tcp_recv: recv error");
	    return -1;
	}
	//printf("iperf_tcp_recv: recv returned %d bytes \n", result);
	//printf("result = %d state = %d, %d = error\n", result, sp->buffer[0], errno);
	message = param_received(sp, param);

	break;

    case STREAM_BEGIN:
    case STREAM_RUNNING:
    case STREAM_END:
    case ALL_STREAMS_END:
	size = sp->settings->blksize;
#ifdef USE_RECV
	do
	{			/* XXX: is the do/while really needed? */
	    printf("iperf_tcp_recv: Calling recv: expecting %d bytes \n", size);
	    result = recv(sp->socket, sp->buffer, size, MSG_WAITALL);

	} while (result == -1 && errno == EINTR);
#else
	result = Nread(sp->socket, sp->buffer, size, 0);	/* XXX: TCP only for the
								 * momment! */
#endif
	if (result == -1)
	{
	    perror("Read error");
	    return -1;
	}
	//printf("iperf_tcp_recv: recv returned %d bytes \n", result);
	sp->result->bytes_received += result;
	break;
    default:
	printf("unexpected state encountered: %d \n", message);
	break;
    }

    return message;
}

/**************************************************************************/
int
iperf_udp_recv(struct iperf_stream * sp)
{
    int       result, message;
    int       size = sp->settings->blksize;
    double    transit = 0, d = 0;
    struct udp_datagram *udp = (struct udp_datagram *) sp->buffer;
    struct timeval arrival_time;

    if (!sp->buffer)
    {
	fprintf(stderr, "receive buffer not allocated \n");
	exit(0);
    }
    do
    {
	result = recv(sp->socket, sp->buffer, size, 0);

    } while (result == -1 && errno == EINTR);

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
iperf_tcp_send(struct iperf_stream * sp)
{
    int       result;
    int       size = sp->settings->blksize;
    struct param_exchange *param = (struct param_exchange *) sp->buffer;
    struct sockaddr dest;

    if (!sp->buffer)
    {
	perror("transmit buffer not allocated");
	return -1;
    }
    strncpy(param->cookie, sp->settings->cookie, 37);
    switch (sp->settings->state)
    {
    case PARAM_EXCHANGE:
	param->state = PARAM_EXCHANGE;
	size = sizeof(struct param_exchange);
	break;

    case STREAM_BEGIN:
	param->state = STREAM_BEGIN;
	size = sp->settings->blksize;
	break;

    case STREAM_END:
	param->state = STREAM_END;
	size = sp->settings->blksize;	/* XXX: this might not be right, will
					 * the last block always be full
					 * size? */
	break;

    case RESULT_REQUEST:
	param->state = RESULT_REQUEST;
	size = sizeof(struct param_exchange);
	break;

    case ALL_STREAMS_END:
	param->state = ALL_STREAMS_END;
	size = sizeof(struct param_exchange);
	break;

    case STREAM_RUNNING:
	param->state = STREAM_RUNNING;
	size = sp->settings->blksize;
	break;
    default:
	printf("State of the stream can't be determined\n");
	break;
    }

    //printf("   in iperf_tcp_send, message type = %d (total = %d bytes) \n", param->state, size);
#ifdef USE_SEND
    result = send(sp->socket, sp->buffer, size, 0);
#else
    result = Nwrite(sp->socket, sp->buffer, size, 0, dest);	/* XXX: TCP only at the
								 * moment! */
#endif
    if (result < 0)
	perror("Write error");
    //printf("   iperf_tcp_send: %d bytes sent \n", result);

    /* change state after 1st send */
    if (sp->settings->state == STREAM_BEGIN)
	sp->settings->state = STREAM_RUNNING;

    if (sp->buffer[0] != STREAM_END)
	/* XXX: check/fix this. Maybe only want STREAM_BEGIN and STREAM_RUNNING */
	sp->result->bytes_sent += size;

    //printf("number bytes sent so far = %d \n", (int)sp->result->bytes_sent);

    return result;
}

/**************************************************************************/
int
iperf_udp_send(struct iperf_stream * sp)
{
    int       result = 0;
    struct timeval before, after;
    int64_t   dtargus;
    int64_t   adjustus = 0;

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

	result = send(sp->socket, sp->buffer, size, 0);

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
struct iperf_test *
iperf_new_test()
{
    struct iperf_test *testp;

    testp = (struct iperf_test *) malloc(sizeof(struct iperf_test));
    if (!testp)
    {
	perror("malloc");
	return (NULL);
    }
    /* initialise everything to zero */
    memset(testp, 0, sizeof(struct iperf_test));

    testp->default_settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memset(testp->default_settings, 0, sizeof(struct iperf_settings));

    /* return an empty iperf_test* with memory alloted. */
    return testp;
}

/**************************************************************************/
void
iperf_defaults(struct iperf_test * testp)
{
    int       i;

    testp->protocol = Ptcp;
    testp->role = 's';
    testp->duration = DURATION;
    testp->server_port = PORT;

    testp->new_stream = iperf_new_tcp_stream;
    testp->stats_callback = iperf_stats_callback;
    testp->reporter_callback = iperf_reporter_callback;

    testp->stats_interval = 0;
    testp->reporter_interval = 0;
    testp->num_streams = 1;

    testp->default_settings->unit_format = 'a';
    testp->default_settings->socket_bufsize = 0;	/* use autotuning */
    testp->default_settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->default_settings->rate = RATE;	/* UDP only */
    testp->default_settings->state = TEST_START;
    testp->default_settings->mss = 0;
    testp->default_settings->bytes = 0;

    for (i = 0; i < 37; i++)
	testp->default_settings->cookie[i] = '\0';
}

/**************************************************************************/

void
iperf_init_test(struct iperf_test * test)
{
    char      ubuf[UNIT_LEN];
    struct iperf_stream *sp;
    int       i, s = 0;

    if (test->role == 's')
    {				/* server */
	/* XXX: looks like this is setting up both TCP and UDP. Why? -blt */
	test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
	if (test->listener_sock_udp < 0)
	    exit(0);

	test->listener_sock_tcp = netannounce(Ptcp, NULL, test->server_port);
	if (test->listener_sock_tcp < 0)
	    exit(0);

	if (set_tcp_windowsize(test->listener_sock_tcp, test->default_settings->socket_bufsize, SO_RCVBUF) < 0)
	{
	    perror("unable to set TCP window");
	}
	/* XXX: why do we do this?? -blt */
	//setnonblocking(test->listener_sock_tcp);
	//setnonblocking(test->listener_sock_udp);

	printf("-----------------------------------------------------------\n");
	printf("Server listening on %d\n", test->server_port);
	int       x;

	/* make sure we got what we asked for */
	if ((x = get_tcp_windowsize(test->listener_sock_tcp, SO_RCVBUF)) < 0)
	    perror("SO_RCVBUF");

	if (test->protocol == Ptcp)
	{
	    {
		if (test->default_settings->socket_bufsize > 0)
		{
		    unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
		    printf("TCP window size: %s\n", ubuf);
		} else
		{
		    printf("Using TCP Autotuning \n");
		}
	    }
	}
	printf("-----------------------------------------------------------\n");

    } else if (test->role == 'c')
    {				/* Client */
	FD_ZERO(&test->write_set);
	FD_SET(s, &test->write_set);

	for (i = 0; i < test->num_streams; i++)
	{
	    s = netdial(test->protocol, test->server_hostname, test->server_port);
	    if (s < 0)
	    {
		fprintf(stderr, "netdial failed\n");
		exit(0);
	    }
	    FD_SET(s, &test->write_set);
	    test->max_fd = (test->max_fd < s) ? s : test->max_fd;

	    sp = test->new_stream(test);
	    sp->socket = s;
	    iperf_init_stream(sp, test);
	    iperf_add_stream(test, sp);

	    if (test->default_settings->state != RESULT_REQUEST)
		connect_msg(sp);
	}
    }
}

/**************************************************************************/
void
iperf_free_test(struct iperf_test * test)
{
    free(test->default_settings);

    close(test->listener_sock_tcp);
    close(test->listener_sock_udp);

    test->streams = NULL;
    test->accept = NULL;
    test->stats_callback = NULL;
    test->reporter_callback = NULL;
    test->new_stream = NULL;
    free(test);
}

/**************************************************************************/
void     *
iperf_stats_callback(struct iperf_test * test)
{
    iperf_size_t cumulative_bytes = 0;
    int       i;

    struct iperf_stream *sp = test->streams;
    struct iperf_stream_result *rp = test->streams->result;
    struct iperf_interval_results *ip, temp;

    printf("in stats_callback: num_streams = %d \n", test->num_streams);
    for (i = 0; i < test->num_streams; i++)
    {
	rp = sp->result;

	if (!rp->interval_results)
	{
	    if (test->role == 'c')
		temp.bytes_transferred = rp->bytes_sent;
	    else
		temp.bytes_transferred = rp->bytes_received;

	    gettimeofday(&temp.interval_time, NULL);

	    temp.interval_duration = timeval_diff(&sp->result->start_time, &temp.interval_time);

	    if (test->tcp_info)
                get_tcpinfo(test);

	    gettimeofday(&sp->result->end_time, NULL);
	    add_interval_list(rp, temp);
	} else
	{
	    ip = sp->result->interval_results;
	    while (1)
	    {
		cumulative_bytes += ip->bytes_transferred;
		if (ip->next != NULL)
		    ip = ip->next;
		else
		    break;
	    }

	    if (test->role == 'c')
		temp.bytes_transferred = rp->bytes_sent - cumulative_bytes;
	    else
		temp.bytes_transferred = rp->bytes_received - cumulative_bytes;

	    gettimeofday(&temp.interval_time, NULL);
	    temp.interval_duration = timeval_diff(&sp->result->start_time, &temp.interval_time);
	    if (test->tcp_info)
                get_tcpinfo(test);

	    gettimeofday(&sp->result->end_time, NULL);
	    add_interval_list(rp, temp);
	}

	/* for debugging */
	/* display_interval_list(rp, test->tcp_info); */
	cumulative_bytes = 0;
	sp = sp->next;
    }

    return 0;
}

/**************************************************************************/
char     *
iperf_reporter_callback(struct iperf_test * test)
{
    int       count = 0, total_packets = 0, lost_packets = 0;
    char      ubuf[UNIT_LEN];
    char      nbuf[UNIT_LEN];
    struct iperf_stream *sp = test->streams;
    iperf_size_t bytes = 0;
    double    start_time, end_time;
    char     *message = (char *) malloc(500);

    /* used to determine the length of reporter buffer */
    while (sp)
    {
	count++;
	sp = sp->next;
    }

    char     *message_final = (char *) malloc((count + 1) * (strlen(report_bw_jitter_loss_header)
	+ strlen(report_bw_jitter_loss_format) + strlen(report_sum_bw_jitter_loss_format)));

    memset(message_final, 0, strlen(message_final));

    struct iperf_interval_results *ip = test->streams->result->interval_results;
    struct iperf_interval_results *ip_prev = ip;

    sp = test->streams;

    if (test->default_settings->state == TEST_RUNNING)
    {
	while (sp)
	{
	    ip = sp->result->interval_results;
	    while (ip->next != NULL)
	    {
		ip_prev = ip;
		ip = ip->next;
	    }

	    bytes += ip->bytes_transferred;
	    unit_snprintf(ubuf, UNIT_LEN, (double) (ip->bytes_transferred), 'A');

	    if (test->streams->result->interval_results->next != NULL)
	    {
		unit_snprintf(nbuf, UNIT_LEN, 
			(double) (ip->bytes_transferred / (ip->interval_duration - ip_prev->interval_duration)),
			      test->default_settings->unit_format);
		sprintf(message, report_bw_format, sp->socket, ip_prev->interval_duration, ip->interval_duration, ubuf, nbuf);

#if defined(linux)
		/* TODO: do something similar to this everywhere */
		sprintf(message, report_tcpInfo, ip->tcpInfo.tcpi_snd_cwnd, ip->tcpInfo.tcpi_snd_ssthresh,
			ip->tcpInfo.tcpi_rcv_ssthresh, ip->tcpInfo.tcpi_unacked, ip->tcpInfo.tcpi_sacked,
			ip->tcpInfo.tcpi_lost, ip->tcpInfo.tcpi_retrans, ip->tcpInfo.tcpi_fackets);
#endif
#if defined(__FreeBSD__)
		sprintf(message, report_tcpInfo, ip->tcpInfo.tcpi_snd_cwnd,
		 ip->tcpInfo.tcpi_snd_ssthresh, ip->tcpInfo.tcpi_rcv_space, ip->tcpInfo.__tcpi_retrans);
#endif


	    } else
	    {
		sprintf(message, report_bw_header);
		strcat(message_final, message);

		unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred / ip->interval_duration), test->default_settings->unit_format);
		sprintf(message, report_bw_format, sp->socket, 0.0, ip->interval_duration, ubuf, nbuf);
	    }
	    strcat(message_final, message);
	    sp = sp->next;
	}


	if (test->num_streams > 1)
	{
	    unit_snprintf(ubuf, UNIT_LEN, (double) (bytes), 'A');

	    if (test->streams->result->interval_results->next != NULL)
	    {
		unit_snprintf(nbuf, UNIT_LEN, (double) (bytes / (ip->interval_duration - ip_prev->interval_duration)),
			      test->default_settings->unit_format);
		sprintf(message, report_sum_bw_format, ip_prev->interval_duration, ip->interval_duration, ubuf, nbuf);
	    } else
	    {
		unit_snprintf(nbuf, UNIT_LEN, (double) (bytes / ip->interval_duration), test->default_settings->unit_format);
		sprintf(message, report_sum_bw_format, 0.0, ip->interval_duration, ubuf, nbuf);
	    }

	    strcat(message_final, message);
	    free(message);
	}
    }
    if (test->default_settings->state == RESULT_REQUEST)
    {
	sp = test->streams;

	while (sp)
	{
	    if (sp->settings->state == STREAM_END)
	    {
		if (test->role == 'c')
		    bytes += sp->result->bytes_sent;
		else
		    bytes += sp->result->bytes_received;

		if (test->protocol == Pudp)
		{
		    total_packets += sp->packet_count;
		    lost_packets += sp->cnt_error;
		}
		start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
		end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);

		if (test->role == 'c')
		{
		    unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_sent), 'A');
		    unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_sent / end_time), test->default_settings->unit_format);

		} else
		{
		    unit_snprintf(ubuf, UNIT_LEN, (double) (sp->result->bytes_received), 'A');
		    unit_snprintf(nbuf, UNIT_LEN, (double) (sp->result->bytes_received / end_time), test->default_settings->unit_format);
		}

		if (test->protocol == Ptcp)
		{
		    sprintf(message, report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
		    strcat(message_final, message);
		} else
		{
		    sprintf(message, report_bw_jitter_loss_format, sp->socket, start_time,
			    end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, sp->packet_count, (double) (100.0 * sp->cnt_error / sp->packet_count));
		    strcat(message_final, message);

		    if (test->role == 'c')
		    {
			sprintf(message, report_datagrams, sp->socket, sp->packet_count);
			strcat(message_final, message);
		    }
		    if (sp->outoforder_packets > 0)
			printf(report_sum_outoforder, start_time, end_time, sp->cnt_error);
		}
	    }
	    sp = sp->next;
	}

	sp = test->streams;

	start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
	end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);

	unit_snprintf(ubuf, UNIT_LEN, (double) bytes, 'A');
	unit_snprintf(nbuf, UNIT_LEN, (double) bytes / end_time, test->default_settings->unit_format);

	if ((test->role == 'c' && test->num_streams > 1) || (test->role == 's'))
	{
	    if (test->protocol == Ptcp)
	    {
		sprintf(message, report_sum_bw_format, start_time, end_time, ubuf, nbuf);
		strcat(message_final, message);
	    } else
	    {
		sprintf(message, report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf, sp->jitter, lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
		strcat(message_final, message);
	    }


	    if ((test->print_mss != 0) && (test->role == 'c'))
	    {
		sprintf(message, "\nThe TCP maximum segment size mss = %d \n", getsock_tcp_mss(sp->socket));
		strcat(message_final, message);
	    }
	    free(message);
	}
    }
    return message_final;
}

/**************************************************************************/
void
iperf_free_stream(struct iperf_test * test, struct iperf_stream * sp)
{
    struct iperf_stream *prev, *start;

    prev = test->streams;
    start = test->streams;

    if (test->streams->socket == sp->socket)
    {
	test->streams = test->streams->next;
    } else
    {
	start = test->streams->next;
	while (1)
	{
	    if (start->socket == sp->socket)
	    {
		prev->next = sp->next;
		break;
	    }
	    if (start->next != NULL)
	    {
		start = start->next;
		prev = prev->next;
	    }
	}
    }

    free(sp->buffer);
    free(sp->settings);
    free(sp->result);
    free(sp->send_timer);
    free(sp);
}

/**************************************************************************/
struct iperf_stream *
iperf_new_stream(struct iperf_test * testp)
{
    int i=0;
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if (!sp)
    {
	perror("malloc");
	return (NULL);
    }
    memset(sp, 0, sizeof(struct iperf_stream));

    sp->buffer = (char *) malloc(testp->default_settings->blksize);
    sp->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    memcpy(sp->settings, testp->default_settings, sizeof(struct iperf_settings));
    sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));

    /* fill in buffer with random stuff */
    /* XXX: probably better to use truely random stuff here */
    for(i=0; i < testp->default_settings->blksize; i++)
        sp->buffer[i] = i%255;

    sp->socket = -1;

    sp->packet_count = 0;
    sp->stream_id = (int) sp;
    sp->jitter = 0.0;
    sp->prev_transit = 0.0;
    sp->outoforder_packets = 0;
    sp->cnt_error = 0;

    sp->send_timer = NULL;

    sp->result->interval_results = NULL;
    sp->result->bytes_received = 0;
    sp->result->bytes_sent = 0;
    gettimeofday(&sp->result->start_time, NULL);

    sp->settings->state = STREAM_BEGIN;
    return sp;
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

    //sp->update_stats = iperf_tcp_update_stats;

    return sp;
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

    printf("1st UDP data  packet for socket %d has arrived \n", sp->socket);
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

/**************************************************************************/
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

	FD_SET(peersock, &test->read_set);
	test->max_fd = (test->max_fd < peersock) ? peersock : test->max_fd;

	sp->socket = peersock;
	printf("in iperf_tcp_accept: tcp_windowsize: %d \n", test->default_settings->socket_bufsize);
	iperf_init_stream(sp, test);
	iperf_add_stream(test, sp);

	if (test->default_settings->state != RESULT_REQUEST)
	    connect_msg(sp);

	return 0;
    }
}

/**************************************************************************/
void
iperf_init_stream(struct iperf_stream * sp, struct iperf_test * testp)
{
    socklen_t len;

    len = sizeof(struct sockaddr_in);

    sp->protocol = testp->protocol;

    if (getsockname(sp->socket, (struct sockaddr *) & sp->local_addr, &len) < 0)
    {
	perror("getsockname");
    }
    if (getpeername(sp->socket, (struct sockaddr *) & sp->remote_addr, &len) < 0)
    {
	perror("getpeername");
    }
    //printf("in init_stream: calling set_tcp_windowsize: %d \n", testp->default_settings->socket_bufsize);
    if (set_tcp_windowsize(sp->socket, testp->default_settings->socket_bufsize,
			   testp->role == 's' ? SO_RCVBUF : SO_SNDBUF) < 0)
	fprintf(stderr, "unable to set window size\n");

    set_socket_options(sp, testp);	/* set other options (TCP_NODELAY,
					 * MSS, etc.) */
}

/**************************************************************************/
int
iperf_add_stream(struct iperf_test * test, struct iperf_stream * sp)
{
    struct iperf_stream *n;

    if (!test->streams)
    {
	test->streams = sp;
	return 1;
    } else
    {
	n = test->streams;
	while (n->next)
	    n = n->next;
	n->next = sp;
	return 1;
    }

    return 0;
}

/**************************************************************************/
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

/**************************************************************************/
void
iperf_run_server(struct iperf_test * test)
{
    struct timeval tv;
    struct iperf_stream *np, *sp;
    int       j, result, message;
    char     *read = NULL;

    FD_ZERO(&test->read_set);
    FD_SET(test->listener_sock_tcp, &test->read_set);
    FD_SET(test->listener_sock_udp, &test->read_set);

    test->max_fd = test->listener_sock_tcp > test->listener_sock_udp ? test->listener_sock_tcp : test->listener_sock_udp;

    test->num_streams = 0;
    test->default_settings->state = TEST_RUNNING;

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
		    message = np->rcv(np);

		    if (message == PARAM_EXCHANGE || message == ACCESS_DENIED)
		    {
			/* copy the received settings into test */
			if (message != ACCESS_DENIED)
			    memcpy(test->default_settings, test->streams->settings, sizeof(struct iperf_settings));

			close(np->socket);
			FD_CLR(np->socket, &test->read_set);
			iperf_free_stream(test, np);
		    }
		    if (message == STREAM_END)
		    {
			np->settings->state = STREAM_END;
			gettimeofday(&np->result->end_time, NULL);
		    }
		    if (message == RESULT_REQUEST)
		    {
			np->settings->state = RESULT_RESPOND;
			np->data = read;
			send_result_to_client(np);
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
		    }
		    if (message == ALL_STREAMS_END)
		    {
			/*
		         * sometimes the server is not getting the STREAM_END
		         * message, hence changing the state of all but last
		         * stream forcefully
		         */
			np = test->streams;
			while (np->next)
			{
			    if (np->settings->state == STREAM_BEGIN)
			    {
				np->settings->state = STREAM_END;
				gettimeofday(&np->result->end_time, NULL);
			    }
			    np = np->next;
			}

			/* This is necessary to preserve reporting format */
			test->protocol = test->streams->protocol;

			test->default_settings->state = RESULT_REQUEST;
			read = test->reporter_callback(test);
			puts(read);
			/* printf("REPORTER CALL + ALL_STREAMS_END\n");  */
		    }
		}		/* end if (FD_ISSET(j, &temp_set)) */
	    }			/* end for (j=0;...) */

	}			/* end else (result>0)   */
    }				/* end while */

}

/**************************************************************************/
void
catcher(int sig)
{
    longjmp(env, sig);
}

/**************************************************************************/
void
iperf_run_client(struct iperf_test * test)
{
    int       i, result = 0;
    struct iperf_stream *sp, *np;
    struct timer *timer, *stats_interval, *reporter_interval;
    char     *read = NULL;
    int64_t   delayus, adjustus, dtargus;
    struct timeval tv;
    int       ret = 0;
    struct sigaction sact;

    tv.tv_sec = 15;		/* timeout interval in seconds */
    tv.tv_usec = 0;

    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;
    sigaction(SIGINT, &sact, NULL);

    if (test->protocol == Pudp)
    {
	dtargus = (int64_t) (test->default_settings->blksize) * SEC_TO_US * 8;
	dtargus /= test->default_settings->rate;

	assert(dtargus != 0);

	delayus = dtargus;
	adjustus = 0;
	printf("%lld adj %lld delay\n", adjustus, delayus);

	sp = test->streams;
	for (i = 0; i < test->num_streams; i++)
	{
	    sp->send_timer = new_timer(0, dtargus);
	    sp = sp->next;
	}
    }
    /* if -n specified, set zero timer */
    if (test->default_settings->bytes == 0)
	timer = new_timer(test->duration, 0);
    else
	timer = new_timer(0, 0);

    if (test->stats_interval != 0)
	stats_interval = new_timer(test->stats_interval, 0);

    if (test->reporter_interval != 0)
	reporter_interval = new_timer(test->reporter_interval, 0);

    if (test->default_settings->bytes == 0)
        printf("Starting Test: %d streams, %d byte blocks, %d second test \n",
		test->num_streams, test->default_settings->blksize, test->duration);
    else
        printf("Starting Test: %d streams, %d byte blocks, %d bytes to send\n",
		test->num_streams, test->default_settings->blksize, (int) test->default_settings->bytes);


    /* send data till the timer expires or bytes sent */
    while (!all_data_sent(test) && !timer->expired(timer))
    {

#ifdef NEED_THIS  /* not sure what this was for, so removed -blt */
	memcpy(&test->temp_set, &test->write_set, sizeof(test->write_set));
        printf("Calling select... \n");
	ret = select(test->max_fd + 1, NULL, &test->write_set, NULL, &tv);
	if (ret < 0)
	    continue;
#endif

	sp = test->streams;
	for (i = 0; i < test->num_streams; i++)
	{
	    result += sp->snd(sp);

	    if (sp->next == NULL)
		break;
	    sp = sp->next;
	}


	if ((test->stats_interval != 0) && stats_interval->expired(stats_interval))
	{
	    test->stats_callback(test);
	    update_timer(stats_interval, test->stats_interval, 0);
	}
	if ((test->reporter_interval != 0) && reporter_interval->expired(reporter_interval))
	{
	    read = test->reporter_callback(test);
	    puts(read);
	    update_timer(reporter_interval, test->reporter_interval, 0);
	}
	/* detecting Ctrl+C */
	if (setjmp(env))
	    break;

    }				/* while outer timer  */

    /* for last interval   */
    test->stats_callback(test);
    read = test->reporter_callback(test);
    puts(read);

    printf("Test Complete. \n");
    /* sending STREAM_END packets */
    sp = test->streams;
    np = sp;
    do
    {
	sp = np;
	sp->settings->state = STREAM_END;
	sp->snd(sp);
	np = sp->next;
    } while (np);
    //printf("Done Sending STREAM_END. \n");

    /* Requesting for result from Server */
    test->default_settings->state = RESULT_REQUEST;
    // receive_result_from_server(test);  /* XXX: current broken? bus error */
    read = test->reporter_callback(test);
    printf("Summary results as measured by the server: \n");
    puts(read);

    //printf("Done getting/printing results. \n");

    /* Deleting all streams - CAN CHANGE FREE_STREAM FN */
    sp = test->streams;
    np = sp;
    do
    {
	sp = np;
	close(sp->socket);
	np = sp->next;
	iperf_free_stream(test, sp);
    } while (np);

    if (test->stats_interval != 0)
	free_timer(stats_interval);
    if (test->reporter_interval != 0)
	free_timer(reporter_interval);
    free_timer(timer);
}

/**************************************************************************/
int
iperf_run(struct iperf_test * test)
{
    test->default_settings->state = TEST_RUNNING;

    switch (test->role)
    {
    case 's':
	iperf_run_server(test);
	return 0;
	break;
    case 'c':
	iperf_run_client(test);
	return 0;
	break;
    default:
	return -1;
	break;
    }
    printf("Done iperf_run. \n");
}


/**************************************************************************/

static struct option longopts[] =
{
    {"client", required_argument, NULL, 'c'},
    {"server", no_argument, NULL, 's'},
    {"time", required_argument, NULL, 't'},
    {"port", required_argument, NULL, 'p'},
    {"parallel", required_argument, NULL, 'P'},
    {"udp", no_argument, NULL, 'u'},
    {"tcpInfo", no_argument, NULL, 'T'},
    {"bandwidth", required_argument, NULL, 'b'},
    {"length", required_argument, NULL, 'l'},
    {"window", required_argument, NULL, 'w'},
    {"interval", required_argument, NULL, 'i'},
    {"bytes", required_argument, NULL, 'n'},
    {"NoDelay", no_argument, NULL, 'N'},
    {"Print-mss", no_argument, NULL, 'm'},
    {"Set-mss", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

/**************************************************************************/

int
main(int argc, char **argv)
{

    char      ch, role;
    struct iperf_test *test;
    int       port = PORT;

#ifdef TEST_PROC_AFFINITY
    /* didnt seem to work.... */
    /*
     * increasing the priority of the process to minimise packet generation
     * delay
     */
    int       rc = setpriority(PRIO_PROCESS, 0, -15);

    if (rc < 0)
    {
	perror("setpriority:");
	printf("setting priority to valid level\n");
	rc = setpriority(PRIO_PROCESS, 0, 0);
    }
    /* setting the affinity of the process  */
    cpu_set_t cpu_set;
    int       affinity = -1;
    int       ncores = 1;

    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
    if (errno)
	perror("couldn't get affinity:");

    if ((ncores = sysconf(_SC_NPROCESSORS_CONF)) <= 0)
	err("sysconf: couldn't get _SC_NPROCESSORS_CONF");

    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0)
	err("couldn't change CPU affinity");
#endif

    test = iperf_new_test();
    iperf_defaults(test); /* sets defaults */

    while ((ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:i:n:mNThM:f:", longopts, NULL)) != -1)
    {
	switch (ch)
	{
	case 'c':
	    test->role = 'c';
	    role = test->role;
	    test->server_hostname = (char *) malloc(strlen(optarg));
	    strncpy(test->server_hostname, optarg, strlen(optarg));
	    break;
	case 'p':
	    test->server_port = atoi(optarg);
	    port = test->server_port;
	    break;
	case 's':
	    test->role = 's';
	    role = test->role;
	    break;
	case 't':
	    test->duration = atoi(optarg);
	    break;
	case 'u':
	    test->protocol = Pudp;
	    test->default_settings->blksize = DEFAULT_UDP_BLKSIZE;
	    test->new_stream = iperf_new_udp_stream;
	    break;
	case 'P':
	    test->num_streams = atoi(optarg);
	    break;
	case 'b':
	    test->default_settings->rate = unit_atof(optarg);
	    break;
	case 'l':
	    test->default_settings->blksize = unit_atoi(optarg);
	    printf("%d is the blksize\n", test->default_settings->blksize);
	    break;
	case 'w':
	    test->default_settings->socket_bufsize = unit_atof(optarg);
	    break;
	case 'i':
	    test->stats_interval = atoi(optarg);
	    test->reporter_interval = atoi(optarg);
	    break;
	case 'n':
	    test->default_settings->bytes = unit_atoi(optarg);
	    printf("total bytes to be transferred = %llu\n", test->default_settings->bytes);
	    break;
	case 'm':
	    test->print_mss = 1;
	    break;
	case 'N':
	    test->no_delay = 1;
	    break;
	case 'M':
	    test->default_settings->mss = atoi(optarg);
	    break;
	case 'f':
	    test->default_settings->unit_format = *optarg;
	    break;
	case 'T':
	    test->tcp_info = 1;
	    break;
	case 'h':
	default:
	    fprintf(stderr, usage_long1);
	    fprintf(stderr, usage_long2);
	    exit(1);
	}
    }


    //printf("in main: calling iperf_init_test \n");
    printf("Connection to port %d on host %s \n", test->server_port, test->server_hostname);
    iperf_init_test(test);

    if (test->role == 'c')  /* if client, send params to server */
    {
	exchange_parameters(test);
        test->streams->settings->state = STREAM_BEGIN;
    }

    //printf("in main: calling iperf_run \n");
    iperf_run(test);
    iperf_free_test(test);

    printf("\niperf Done.\n");
    exit(0);
}
