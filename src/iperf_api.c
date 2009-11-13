
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
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

jmp_buf   env;			/* to handle longjmp on signal */

/*************************************************************/

/*
 * check to see if client has sent the requested number of bytes to the
 * server yet
 */

/*
 * XXX: should probably just compute this as we go and store it in the
 * iperf_test structure -blt
 */
int
all_data_sent(struct iperf_test * test)
{
    if (test->default_settings->bytes == 0)
	return 0;
    else
    {
	uint64_t  total_bytes = 0;
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

/**
 * exchange_parameters - handles the param_Exchange part for client
 *
 */

void
exchange_parameters(struct iperf_test * test)
{
    int       result;
    struct iperf_stream *sp;
    struct param_exchange *param;

    printf("in exchange_parameters \n");
    sp = test->streams;
    sp->settings->state = PARAM_EXCHANGE;
    param = (struct param_exchange *) sp->buffer;

    get_uuid(test->default_settings->cookie);
    strncpy(param->cookie, test->default_settings->cookie, COOKIE_SIZE);
    //printf("client cookie: %s \n", param->cookie);

    /* setting up exchange parameters  */
    param->state = PARAM_EXCHANGE;
    param->blksize = test->default_settings->blksize;
    param->recv_window = test->default_settings->socket_bufsize;
    param->send_window = test->default_settings->socket_bufsize;
    param->format = test->default_settings->unit_format;

#ifdef OLD_WAY
    //printf(" sending exchange params: size = %d \n", (int) sizeof(struct param_exchange));
    /* XXX: can we use iperf_tcp_send for this? that would be cleaner */
    result = send(sp->socket, sp->buffer, sizeof(struct param_exchange), 0);
    if (result < 0)
	perror("Error sending exchange params to server");

    //printf("result = %d state = %d, error = %d \n", result, sp->buffer[0], errno);

    /* get answer back from server */
    do
    {
	//printf("exchange_parameters: reading result from server .. \n");
	result = recv(sp->socket, sp->buffer, sizeof(struct param_exchange), 0);
    } while (result == -1 && errno == EINTR);
#else
    printf(" sending exchange params: size = %d \n", (int) sizeof(struct param_exchange));
    result = sp->snd(sp);
    if (result < 0)
	perror("Error sending exchange params to server");
    result = Nread(sp->socket, sp->buffer, sizeof(struct param_exchange), Ptcp);
#endif

    if (result < 0)
	perror("Error getting exchange params ack from server");
    if (result > 0 && sp->buffer[0] == ACCESS_DENIED)
    {
	fprintf(stderr, "Busy server Detected. Try again later. Exiting.\n");
	exit(-1);
    }
    return;
}

/*************************************************************/
/**
 * add_to_interval_list -- adds new interval to the interval_list
 *
 */

void
add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results * new)
{
    struct iperf_interval_results *ip = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));

    ip->bytes_transferred = new->bytes_transferred;
    ip->interval_duration = new->interval_duration;
#if defined(linux) || defined(__FreeBSD__)
    memcpy(&ip->tcpInfo, &new->tcpInfo, sizeof(struct tcp_info));
#endif
    ip->next = NULL;

    //printf("add_to_interval_list: Mbytes = %d, duration = %f \n", (int) (ip->bytes_transferred / 1000000), ip->interval_duration);

    if (!rp->interval_results)	/* if 1st interval */
    {
	rp->interval_results = ip;
	rp->last_interval_results = ip;
    } else
    {
	/* add to end of list */
	rp->last_interval_results->next = ip;
	rp->last_interval_results = ip;
    }
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
	printf("Interval = %f\tMBytes transferred = %u\n", n->interval_duration, (uint) (n->bytes_transferred / MB));
	if (tflag)
	    print_tcpinfo(n);
	n = n->next;
    }
}

/************************************************************/

/**
 * receive_result_from_server - Receives result from server
 */

void
receive_result_from_server(struct iperf_test * test)
{
    int       result;
    struct iperf_stream *sp;
    int       size = 0;
    char     *buf = NULL;

    printf("in receive_result_from_server \n");
    sp = test->streams;
    size = MAX_RESULT_STRING;

    buf = (char *) malloc(size);

    printf("receive_result_from_server: send ALL_STREAMS_END to server \n");
    sp->settings->state = ALL_STREAMS_END;
    sp->snd(sp);		/* send message to server */

    printf("receive_result_from_server: send RESULT_REQUEST to server \n");
    sp->settings->state = RESULT_REQUEST;
    sp->snd(sp);		/* send message to server */

    /* receive from server */

    printf("reading results (size=%d) back from server \n", size);
    do
    {
	result = recv(sp->socket, buf, size, 0);
    } while (result == -1 && errno == EINTR);
    printf("Got size of results from server: %d \n", result);

    printf(server_reporting, sp->socket);
    puts(buf);			/* prints results */

}

/*************************************************************/

/**
 * connect_msg -- displays connection message
 * denoting sender/receiver details
 *
 */

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
/**
 * Display -- Displays results for test
 * Mainly for DEBUG purpose
 *
 */

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
		printf("position-%d\tsp=%d\tsocket=%d\tMbytes sent=%u\n", count++, (int) n, n->socket, (uint) (n->result->bytes_sent / MB));
	    else
		printf("position-%d\tsp=%d\tsocket=%d\tMbytes received=%u\n", count++, (int) n, n->socket, (uint) (n->result->bytes_received / MB));

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

struct iperf_test *
iperf_new_test()
{
    struct iperf_test *testp;

    //printf("in iperf_new_test: reinit default settings \n");
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
    memset(testp->default_settings->cookie, '\0', COOKIE_SIZE);
}

/**************************************************************************/

void
iperf_init_test(struct iperf_test * test)
{
    char      ubuf[UNIT_LEN];
    struct iperf_stream *sp;
    int       i, s = 0;

    printf("in iperf_init_test \n");
    if (test->role == 's')
    {				/* server */
	if (test->protocol == Pudp)
	{
	    test->listener_sock_udp = netannounce(Pudp, NULL, test->server_port);
	    if (test->listener_sock_udp < 0)
		exit(0);
	}
	/* always create TCP connection for control messages */
	test->listener_sock_tcp = netannounce(Ptcp, NULL, test->server_port);
	if (test->listener_sock_tcp < 0)
	    exit(0);

	if (test->protocol == Ptcp)
	{
	    if (set_tcp_windowsize(test->listener_sock_tcp, test->default_settings->socket_bufsize, SO_RCVBUF) < 0)
		perror("unable to set TCP window");
	}
	/* make sure that accept call does not block */
	setnonblocking(test->listener_sock_tcp);
	setnonblocking(test->listener_sock_udp);

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

	/*
         * XXX: I think we need to create a TCP control socket here too for
         * UDP mode -blt
         */
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

	    connect_msg(sp);	/* print connection established message */
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

/**
 * iperf_stats_callback -- handles the statistic gathering for both the client and server
 *
 *returns void *
 *
 */


void     *
iperf_stats_callback(struct iperf_test * test)
{
    iperf_size_t cumulative_bytes = 0;
    int       i;

    struct iperf_stream *sp = test->streams;
    struct iperf_stream_result *rp = test->streams->result;
    struct iperf_interval_results *ip = NULL, temp;

    //printf("in stats_callback: num_streams = %d \n", test->num_streams);
    for (i = 0; i < test->num_streams; i++)
    {
	rp = sp->result;

	if (!rp->interval_results)	/* 1st entry in list */
	{
	    if (test->role == 'c')
		temp.bytes_transferred = rp->bytes_sent;
	    else
		temp.bytes_transferred = rp->bytes_received;
	} else
	{
	    ip = sp->result->interval_results;
	    cumulative_bytes = 0;
	    while (ip->next != NULL)
	    {			/* compute cumulative_bytes over all
				 * intervals */
		cumulative_bytes += ip->bytes_transferred;
		ip = ip->next;
	    }
	    /* compute number of bytes transferred on this stream */
	    /* XXX: fix this calculation: this is really awkward */
	    if (test->role == 'c')
		temp.bytes_transferred = rp->bytes_sent - cumulative_bytes;
	    else
		temp.bytes_transferred = rp->bytes_received - cumulative_bytes;
	}

	gettimeofday(&sp->result->end_time, NULL);
	temp.interval_duration = timeval_diff(&sp->result->start_time, &sp->result->end_time);
	if (test->tcp_info)
	    get_tcpinfo(test, &temp);
	add_to_interval_list(rp, &temp);

	/* for debugging */
	/* display_interval_list(rp, test->tcp_info); */
	sp = sp->next;
    }				/* for each stream */

    return 0;
}

/**************************************************************************/

/**
 * iperf_reporter_callback -- handles the report printing
 *
 *returns report
 *
 */

char     *
iperf_reporter_callback(struct iperf_test * test)
{
    int       total_packets = 0, lost_packets = 0, curr_state = 0;
    int       first_stream = 1;
    char      ubuf[UNIT_LEN];
    char      nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes = 0;
    double    start_time, end_time;
    struct iperf_interval_results *ip = NULL, *ip_prev = NULL;
    char     *message = (char *) malloc(MAX_RESULT_STRING);
    char     *message_final = (char *) malloc(MAX_RESULT_STRING);

    memset(message_final, 0, strlen(message_final));

    sp = test->streams;
    curr_state = sp->settings->state;
    //printf("in iperf_reporter_callback: state = %d \n", curr_state);

    if (curr_state == TEST_RUNNING || curr_state == STREAM_RUNNING)
    {
	/* print interval results */
	while (sp)
	{			/* for each stream */
	    ip = sp->result->interval_results;
	    if (ip == NULL)
	    {
		printf("iperf_reporter_callback Error: interval_results = NULL \n");
		break;
	    }
	    while (ip->next != NULL)	/* find end of list. XXX: why not
					 * just keep track of this pointer?? */
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

	    } else
	    {
		if (first_stream)	/* only print header for 1st stream */
		{
		    sprintf(message, report_bw_header);
		    safe_strcat(message_final, message);
		    first_stream = 0;
		}
		unit_snprintf(nbuf, UNIT_LEN, (double) (ip->bytes_transferred / ip->interval_duration), test->default_settings->unit_format);
		sprintf(message, report_bw_format, sp->socket, 0.0, ip->interval_duration, ubuf, nbuf);
	    }
	    if (strlen(message_final) + strlen(message) < MAX_RESULT_STRING)
		safe_strcat(message_final, message);
	    else
	    {
		printf("Error: results string too long \n");
		return NULL;
	    }

	    if (test->tcp_info)
	    {
		build_tcpinfo_message(ip, message);
		safe_strcat(message_final, message);
	    }
	    //printf("reporter_callback: built interval string: %s \n", message_final);
	    sp = sp->next;
	}			/* while (sp) */


	if (test->num_streams > 1)	/* sum of all streams */
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
	    safe_strcat(message_final, message);

#ifdef NOT_DONE			/* is it usful to figure out a way so sum
				 * TCP_info acrross multiple streams? */
	    if (test->tcp_info)
	    {
		build_tcpinfo_message(ip, message);
		safe_strcat(message_final, message);
	    }
#endif
	}
    } else
    {
	if (curr_state == ALL_STREAMS_END || curr_state == RESULT_REQUEST)
	{
	    /* if TEST_RUNNING */
	    sp = test->streams;

	    while (sp)
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
		    safe_strcat(message_final, message);
		    if (test->tcp_info)
		    {
			printf("Final TCP_INFO results: \n");
			build_tcpinfo_message(ip, message);
			safe_strcat(message_final, message);
		    }
		} else
		{		/* UDP mode */
		    sprintf(message, report_bw_jitter_loss_format, sp->socket, start_time,
			    end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, sp->packet_count, (double) (100.0 * sp->cnt_error / sp->packet_count));
		    safe_strcat(message_final, message);

		    if (test->role == 'c')
		    {
			sprintf(message, report_datagrams, sp->socket, sp->packet_count);
			safe_strcat(message_final, message);
		    }
		    if (sp->outoforder_packets > 0)
			printf(report_sum_outoforder, start_time, end_time, sp->cnt_error);
		}

		sp = sp->next;
	    }
	}			/* while (sp) */
	sp = test->streams;	/* reset to first socket */

	start_time = timeval_diff(&sp->result->start_time, &sp->result->start_time);
	end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);

	unit_snprintf(ubuf, UNIT_LEN, (double) bytes, 'A');
	unit_snprintf(nbuf, UNIT_LEN, (double) bytes / end_time, test->default_settings->unit_format);

	if ((test->role == 'c' || (test->role == 's')) && test->num_streams > 1)
	{
	    if (test->protocol == Ptcp)
	    {
		sprintf(message, report_sum_bw_format, start_time, end_time, ubuf, nbuf);
		safe_strcat(message_final, message);
	    } else
	    {
		sprintf(message, report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf, sp->jitter, lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
		safe_strcat(message_final, message);
	    }


	    if ((test->print_mss != 0) && (test->role == 'c'))
	    {
		sprintf(message, "\nThe TCP maximum segment size mss = %d \n", getsock_tcp_mss(sp->socket));
		safe_strcat(message_final, message);
	    }
	}
    }
    free(message);
    return message_final;
}

/**************************************************************************/
void
safe_strcat(char *s1, char *s2)
{
    //printf(" adding string %s to end of string %s \n", s1, s1);
    if (strlen(s1) + strlen(s2) < MAX_RESULT_STRING)
	strcat(s1, s2);
    else
    {
	printf("Error: results string too long \n");
	exit(-1);		/* XXX: should return an error instead! */
	//return -1;
    }
}

/**************************************************************************/
void
iperf_free_stream(struct iperf_stream * sp)
{
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
    int       i = 0;
    struct iperf_stream *sp;

    //printf("in iperf_new_stream \n");
    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if (!sp)
    {
	perror("malloc");
	return (NULL);
    }
    memset(sp, 0, sizeof(struct iperf_stream));

    printf("iperf_new_stream: Allocating new stream buffer: size = %d \n", testp->default_settings->blksize);
    sp->buffer = (char *) malloc(testp->default_settings->blksize);
    sp->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    /* make a per stream copy of default_settings in each stream structure */
    memcpy(sp->settings, testp->default_settings, sizeof(struct iperf_settings));
    sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));

    /* fill in buffer with random stuff */
    srandom(time(0));
    for (i = 0; i < testp->default_settings->blksize; i++)
	sp->buffer[i] = random();

    sp->socket = -1;

    sp->packet_count = 0;
    sp->stream_id = (int) sp;
    sp->jitter = 0.0;
    sp->prev_transit = 0.0;
    sp->outoforder_packets = 0;
    sp->cnt_error = 0;

    sp->send_timer = NULL;
    sp->next = NULL;

    sp->result->interval_results = NULL;
    sp->result->last_interval_results = NULL;
    sp->result->bytes_received = 0;
    sp->result->bytes_sent = 0;
    gettimeofday(&sp->result->start_time, NULL);

    sp->settings->state = STREAM_BEGIN;
    return sp;
}

/**************************************************************************/
void
iperf_init_stream(struct iperf_stream * sp, struct iperf_test * testp)
{
    socklen_t len;

    len = sizeof(struct sockaddr_in);

    if (getsockname(sp->socket, (struct sockaddr *) & sp->local_addr, &len) < 0)
    {
	perror("getsockname");
    }
    if (getpeername(sp->socket, (struct sockaddr *) & sp->remote_addr, &len) < 0)
    {
	perror("getpeername");
    }
    //printf("in init_stream: calling set_tcp_windowsize: %d \n", testp->default_settings->socket_bufsize);
    if (testp->protocol == Ptcp)
    {
	if (set_tcp_windowsize(sp->socket, testp->default_settings->socket_bufsize,
			    testp->role == 's' ? SO_RCVBUF : SO_SNDBUF) < 0)
	    fprintf(stderr, "unable to set window size\n");

	/* set TCP_NODELAY and TCP_MAXSEG if requested */
	set_tcp_options(sp->socket, testp->no_delay, testp->default_settings->mss);
    }
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
    char     *result_string = NULL;
    char     *prot = NULL;
    int64_t   delayus, adjustus, dtargus;
    struct timeval tv;
    struct sigaction sact;

    printf("in iperf_run_client \n");
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
	printf("iperf_run_client: adjustus: %lld, delayus: %lld \n", adjustus, delayus);

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

    if (test->protocol == Pudp)
	prot = "UDP";
    else
	prot = "TCP";
    if (test->default_settings->bytes == 0)
	printf("Starting Test: protocol: %s, %d streams, %d byte blocks, %d second test \n",
	       prot, test->num_streams, test->default_settings->blksize, test->duration);
    else
	printf("Starting Test: protocol: %s, %d streams, %d byte blocks, %d bytes to send\n",
	       prot, test->num_streams, test->default_settings->blksize, (int) test->default_settings->bytes);

    /* send data till the timer expires or bytes sent */
    while (!all_data_sent(test) && !timer->expired(timer))
    {
	sp = test->streams;
	for (i = 0; i < test->num_streams; i++)
	{
	    //printf("sending data to stream %d \n", i);
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
	    result_string = test->reporter_callback(test);
	    //printf("interval expired: printing results: \n");
	    puts(result_string);
	    update_timer(reporter_interval, test->reporter_interval, 0);
	}
	/* detecting Ctrl+C */
	if (setjmp(env))
	    break;

    }				/* while outer timer  */
    /* show last interval if necessary */
    test->stats_callback(test);
    result_string = test->reporter_callback(test);
    puts(result_string);


    printf("Test Complete. \n");
    /* send STREAM_END packets */
    np = test->streams;
    do
    {				/* send STREAM_END to all sockets */
	sp = np;
	sp->settings->state = STREAM_END;
	printf("sending state = STREAM_END to stream %d \n", sp->socket);
	result = sp->snd(sp);
	if (result < 0)
	    break;
	np = sp->next;
    } while (np);
    //printf("Done Sending STREAM_END. \n");

    /* send ALL_STREAMS_END packet to 1st socket */
    sp = test->streams;
    sp->settings->state = ALL_STREAMS_END;
    sp->snd(sp);
    //printf("Done Sending ALL_STREAMS_END. \n");

    /* show and final summary */
    test->stats_callback(test);
    result_string = test->reporter_callback(test);
    puts(result_string);

    /* Requesting for result from Server */
    test->default_settings->state = RESULT_REQUEST;
    //receive_result_from_server(test);	/* XXX: currently broken! */
    //result_string = test->reporter_callback(test);
    //printf("Summary results as measured by the server: \n");
    //puts(result_string);

    //printf("Done getting/printing results. \n");

    printf("send TEST_END to server \n");
    sp->settings->state = TEST_END;
    sp->snd(sp);		/* send message to server */

    /* Deleting all streams - CAN CHANGE FREE_STREAM FN */
    sp = test->streams;
    np = sp;
    do
    {
	sp = np;
	close(sp->socket);
	np = sp->next;
	iperf_free_stream(sp);
    } while (np);

    if (test->stats_interval != 0)
	free_timer(stats_interval);
    if (test->reporter_interval != 0)
	free_timer(reporter_interval);
    free_timer(timer);
}
