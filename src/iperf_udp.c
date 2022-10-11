/*
 * iperf, Copyright (c) 2014-2022, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/time.h>
#include <sys/select.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_udp.h"
#include "timer.h"
#include "net.h"
#include "cjson.h"
#include "portable_endian.h"

#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#else
# ifndef PRIu64
#  if sizeof(long) == 8
#   define PRIu64		"lu"
#  else
#   define PRIu64		"llu"
#  endif
# endif
#endif

/* iperf_udp_recv
 *
 * receives the data for UDP
 */
int
iperf_udp_recv(struct iperf_stream *sp)
{
    uint32_t  sec, usec;
    uint64_t  pcount;
    int       r;
    int       size = sp->settings->blksize;
    int       first_packet = 0;
    double    transit = 0, d = 0;
    struct iperf_time sent_time, arrival_time, temp_time;

    r = Nread(sp->socket, sp->buffer, size, Pudp);

    /*
     * If we got an error in the read, or if we didn't read anything
     * because the underlying read(2) got a EAGAIN, then skip packet
     * processing.
     */
    if (r <= 0)
        return r;

    /* Only count bytes received while we're in the correct state. */
    if (sp->test->state == TEST_RUNNING) {

	/*
	 * For jitter computation below, it's important to know if this
	 * packet is the first packet received.
	 */
	if (sp->result->bytes_received == 0) {
	    first_packet = 1;
	}

	sp->result->bytes_received += r;
	sp->result->bytes_received_this_interval += r;

	/* Dig the various counters out of the incoming UDP packet */
	if (sp->test->udp_counters_64bit) {
	    memcpy(&sec, sp->buffer, sizeof(sec));
	    memcpy(&usec, sp->buffer+4, sizeof(usec));
	    memcpy(&pcount, sp->buffer+8, sizeof(pcount));
	    sec = ntohl(sec);
	    usec = ntohl(usec);
	    pcount = be64toh(pcount);
	    sent_time.secs = sec;
	    sent_time.usecs = usec;
	}
	else {
	    uint32_t pc;
	    memcpy(&sec, sp->buffer, sizeof(sec));
	    memcpy(&usec, sp->buffer+4, sizeof(usec));
	    memcpy(&pc, sp->buffer+8, sizeof(pc));
	    sec = ntohl(sec);
	    usec = ntohl(usec);
	    pcount = ntohl(pc);
	    sent_time.secs = sec;
	    sent_time.usecs = usec;
	}

	if (sp->test->debug_level >= DEBUG_LEVEL_DEBUG)
	    fprintf(stderr, "pcount %" PRIu64 " packet_count %d\n", pcount, sp->packet_count);

	/*
	 * Try to handle out of order packets.  The way we do this
	 * uses a constant amount of storage but might not be
	 * correct in all cases.  In particular we seem to have the
	 * assumption that packets can't be duplicated in the network,
	 * because duplicate packets will possibly cause some problems here.
	 *
	 * First figure out if the sequence numbers are going forward.
	 * Note that pcount is the sequence number read from the packet,
	 * and sp->packet_count is the highest sequence number seen so
	 * far (so we're expecting to see the packet with sequence number
	 * sp->packet_count + 1 arrive next).
	 */
	if (pcount >= sp->packet_count + 1) {

	    /* Forward, but is there a gap in sequence numbers? */
	    if (pcount > sp->packet_count + 1) {
		/* There's a gap so count that as a loss. */
		sp->cnt_error += (pcount - 1) - sp->packet_count;
	    }
	    /* Update the highest sequence number seen so far. */
	    sp->packet_count = pcount;
	} else {

	    /*
	     * Sequence number went backward (or was stationary?!?).
	     * This counts as an out-of-order packet.
	     */
	    sp->outoforder_packets++;

	    /*
	     * If we have lost packets, then the fact that we are now
	     * seeing an out-of-order packet offsets a prior sequence
	     * number gap that was counted as a loss.  So we can take
	     * away a loss.
	     */
	    if (sp->cnt_error > 0)
		sp->cnt_error--;

	    /* Log the out-of-order packet */
	    if (sp->test->debug)
		fprintf(stderr, "OUT OF ORDER - incoming packet sequence %" PRIu64 " but expected sequence %d on stream %d\n", pcount, sp->packet_count + 1, sp->socket);
	}

	/*
	 * jitter measurement
	 *
	 * This computation is based on RFC 1889 (specifically
	 * sections 6.3.1 and A.8).
	 *
	 * Note that synchronized clocks are not required since
	 * the source packet delta times are known.  Also this
	 * computation does not require knowing the round-trip
	 * time.
	 */
	iperf_time_now(&arrival_time);

	iperf_time_diff(&arrival_time, &sent_time, &temp_time);
	transit = iperf_time_in_secs(&temp_time);

	/* Hack to handle the first packet by initializing prev_transit. */
	if (first_packet)
	    sp->prev_transit = transit;

	d = transit - sp->prev_transit;
	if (d < 0)
	    d = -d;
	sp->prev_transit = transit;
	sp->jitter += (d - sp->jitter) / 16.0;
    }
    else {
	if (sp->test->debug)
	    iperf_printf(sp->test, "Late receive, state = %d\n", sp->test->state);
    }

    return r;
}


/* iperf_udp_send
 *
 * sends the data for UDP
 */
int
iperf_udp_send(struct iperf_stream *sp)
{
    int r;
    int       size = sp->settings->blksize;
    struct iperf_time before;

    iperf_time_now(&before);

    ++sp->packet_count;

    if (sp->test->udp_counters_64bit) {

	uint32_t  sec, usec;
	uint64_t  pcount;

	sec = htonl(before.secs);
	usec = htonl(before.usecs);
	pcount = htobe64(sp->packet_count);

	memcpy(sp->buffer, &sec, sizeof(sec));
	memcpy(sp->buffer+4, &usec, sizeof(usec));
	memcpy(sp->buffer+8, &pcount, sizeof(pcount));

    }
    else {

	uint32_t  sec, usec, pcount;

	sec = htonl(before.secs);
	usec = htonl(before.usecs);
	pcount = htonl(sp->packet_count);

	memcpy(sp->buffer, &sec, sizeof(sec));
	memcpy(sp->buffer+4, &usec, sizeof(usec));
	memcpy(sp->buffer+8, &pcount, sizeof(pcount));

    }

    r = Nwrite(sp->socket, sp->buffer, size, Pudp);

    if (r < 0)
	return r;

    sp->result->bytes_sent += r;
    sp->result->bytes_sent_this_interval += r;

    if (sp->test->debug_level >=  DEBUG_LEVEL_DEBUG)
	printf("sent %d bytes of %d, total %" PRIu64 "\n", r, sp->settings->blksize, sp->result->bytes_sent);

    return r;
}


/**************************************************************************/

/*
 * The following functions all have to do with managing UDP data sockets.
 * UDP of course is connectionless, so there isn't really a concept of
 * setting up a connection, although connect(2) can (and is) used to
 * bind the remote end of sockets.  We need to simulate some of the
 * connection management that is built-in to TCP so that each side of the
 * connection knows about each other before the real data transfers begin.
 */

/*
 * Set and verify socket buffer sizes.
 * Return 0 if no error, -1 if an error, +1 if socket buffers are
 * potentially too small to hold a message.
 */
int
iperf_udp_buffercheck(struct iperf_test *test, int s)
{
    int rc = 0;
    int sndbuf_actual, rcvbuf_actual;

    /*
     * Set socket buffer size if requested.  Do this for both sending and
     * receiving so that we can cover both normal and --reverse operation.
     */
    int opt;
    socklen_t optlen;

    if ((opt = test->settings->socket_bufsize)) {
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0) {
            i_errno = IESETBUF;
            return -1;
        }
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
            i_errno = IESETBUF;
            return -1;
        }
    }

    /* Read back and verify the sender socket buffer size */
    optlen = sizeof(sndbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &sndbuf_actual, &optlen) < 0) {
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("SNDBUF is %u, expecting %u\n", sndbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > sndbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }
    if (test->settings->blksize > sndbuf_actual) {
	char str[WARN_STR_LEN];
	snprintf(str, sizeof(str),
		 "Block size %d > sending socket buffer size %d",
		 test->settings->blksize, sndbuf_actual);
	warning(str);
	rc = 1;
    }

    /* Read back and verify the receiver socket buffer size */
    optlen = sizeof(rcvbuf_actual);
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen) < 0) {
	i_errno = IESETBUF;
	return -1;
    }
    if (test->debug) {
	printf("RCVBUF is %u, expecting %u\n", rcvbuf_actual, test->settings->socket_bufsize);
    }
    if (test->settings->socket_bufsize && test->settings->socket_bufsize > rcvbuf_actual) {
	i_errno = IESETBUF2;
	return -1;
    }
    if (test->settings->blksize > rcvbuf_actual) {
	char str[WARN_STR_LEN];
	snprintf(str, sizeof(str),
		 "Block size %d > receiving socket buffer size %d",
		 test->settings->blksize, rcvbuf_actual);
	warning(str);
	rc = 1;
    }

    if (test->json_output) {
	cJSON_AddNumberToObject(test->json_start, "sock_bufsize", test->settings->socket_bufsize);
	cJSON_AddNumberToObject(test->json_start, "sndbuf_actual", sndbuf_actual);
	cJSON_AddNumberToObject(test->json_start, "rcvbuf_actual", rcvbuf_actual);
    }

    return rc;
}



/*
 * iperf_udp_bind_to_accepted
 *
 * Bind tockt to address from accepted message
 */
int
iperf_udp_bind_to_accepted(struct iperf_test *test, int s, struct sockaddr_storage *sa_peer, socklen_t sa_peer_len)
{

    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "Binding socket %d to remote address in a received packet.\n", s);
    }

    /* Use the address from the received packet to bind the remote side of the socket. */
    if (connect(s, (struct sockaddr *) sa_peer, sa_peer_len) < 0) {
        i_errno = IESTREAMACCEPT;
        return -1;
    }

    /* Check and set socket buffer sizes */
    if (iperf_udp_buffercheck(test, s) < 0) {
	return -1;
    }

    /*
     * If the socket buffer was too small, but it was the default
     * size, then try explicitly setting it to something larger.
     */
    if (test->settings->socket_bufsize == 0) {
        char str[WARN_STR_LEN];
        int bufsize = test->settings->blksize + UDP_BUFFER_EXTRA;
        snprintf(str, sizeof(str), "Increasing socket buffer size to %d", bufsize);
        warning(str);
        test->settings->socket_bufsize = bufsize;
        if (iperf_udp_buffercheck(test, s) < 0) {
            return -1;
        }
    }

#if defined(HAVE_SO_MAX_PACING_RATE)
    /* If socket pacing is specified, try it. */
    if (test->settings->fqrate) {
	/* Convert bits per second to bytes per second */
	unsigned int fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		printf("Setting fair-queue socket pacing to %u\n", fqrate);
	    }
	    if (setsockopt(s, SOL_SOCKET, SO_MAX_PACING_RATE, &fqrate, sizeof(fqrate)) < 0) {
		warning("Unable to set socket pacing");
	    }
	}
    }
#endif /* HAVE_SO_MAX_PACING_RATE */
    {
	unsigned int rate = test->settings->rate / 8;
	if (rate > 0) {
	    if (test->debug) {
		printf("Setting application pacing to %u\n", rate);
	    }
	}
    }

    return 0;
} /* iperf_udp_bind_to_accepted */


/*
 * iperf_udp_accept
 *
 * Accepts a new UDP "connection"
 */
int
iperf_udp_accept(struct iperf_test *test)
{
    struct sockaddr_storage sa_peer;
    unsigned int buf;
    socklen_t len;
    int       sz, s;

    /*
     * Get the current outstanding socket.  This socket will be used to handle
     * data transfers and a new "listening" socket will be created.
     */
    s = test->prot_listener;

    /*
     * Grab the UDP packet sent by the client.  From that we can extract the
     * client's address, and then use that information to bind the remote side
     * of the socket to the client.
     */
    len = sizeof(sa_peer);
    if ((sz = recvfrom(test->prot_listener, &buf, sizeof(buf), 0, (struct sockaddr *) &sa_peer, &len)) < 0) {
        i_errno = IESTREAMACCEPT;
        return -1;
    }

    if (test->debug) {
        iperf_printf(test, "Accepted Connect message of size %d (of %ld) with msg_id=x%x\n", sz, sizeof(buf), buf);
    }

    /* bind the remote side of the socket to the client */
    if (iperf_udp_bind_to_accepted(test, s, &sa_peer, len) < 0) {
        return -1;
    }

    /*
     * Create a new "listening" socket to replace the one we were using before.
     */
    test->prot_listener = netannounce(test->settings->domain, Pudp, test->bind_address, test->bind_dev, test->server_port);
    if (test->prot_listener < 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    FD_SET(test->prot_listener, &test->read_set);
    test->max_fd = (test->max_fd < test->prot_listener) ? test->prot_listener : test->max_fd;

    /* Let the client know we're ready to "accept" another UDP "stream" */
    if (iperf_udp_send_connect_msg(test, s, UDP_CONNECT_REPLY, 0) < 0) {
        return -1;
    }

    return s;
} /* iperf_udp_accept */




/*
 * iperf_udp_send_connect_msg
 *
 * Send UDP connect related messages with repeats
 */
int
iperf_udp_send_connect_msg(struct iperf_test *test, int s, int msg_type, int repeat_flag)
{
    unsigned int buf;
    int repeats_num;

    repeats_num = (repeat_flag) ? test->settings->udp_connect_retries : 1;
    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "Sending %d UDP connection messages of type=x%x to Socket %d\n", repeats_num, msg_type, s);
    }

    buf = msg_type;

    /* Check only first `write` status, as socket may be closed by the receiver
       after the first message was received. */
    if (write(s, &buf, sizeof(buf)) < 0) {
        i_errno = IESTREAMCNCTSEND;
        return -1;
    }
    if (repeat_flag) {       
        while (--repeats_num > 0) {
            if (write(s, &buf, sizeof(buf)) < 0) {
                // do nothing on failure here
            }
        }
    }

    return 0;
}


/*
 * iperf_udp_acceppt_all_streams_connected_msgs
 *
 * Accepts the "all UDP streams connected" msgs/replies. 
 * Return value: >0 sucess, 0 - no msg received.
 * 
 * Until the all msgs are received, accept input from all streams to
 * discard connection retries messages - making sure connect retries will not be read
 * later as test data sent from the client.  Receive from `control_socket` only when no
 * stream msgs are available (assuming the connection retries and the acks are received
 * in sending order).
 */
unsigned int
iperf_udp_acceppt_all_streams_connected_msgs(struct iperf_test *test, int msg_type, int control_socket, struct sockaddr_storage *out_sa_peer, socklen_t *out_sa_peer_len)
{
    struct sockaddr_storage sa_peer;
    socklen_t sa_peer_len;
    unsigned int all_connected_count;
    int sz, result;
    unsigned int buf;
    struct timeval timeout;
    fd_set read_set, init_read_set;
    struct iperf_stream *sp;
    int max_fd;
    int reply_to_discarded_connect_msg;

    /* this function is not applicable for the legacy UDP streams connect protocol */
    if (test->settings->udp_connect_retries < 2) {
        return 1; // functiona is NA so return success
    }

    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "Receiving all streams connected msgs/replies from socket %d\n", control_socket);
    }

    /* receive only from test streams and not from other sockets */
    FD_ZERO(&init_read_set);
    FD_SET(control_socket, &init_read_set);
    max_fd = control_socket;
    SLIST_FOREACH(sp, &test->streams, streams) {
        /* `control_socket` may or may not in the streams list, so don't add it again */
        if (sp->socket != control_socket) {
            FD_SET(sp->socket, &init_read_set);
            if (sp->socket > max_fd)
                max_fd = sp->socket;
        }
    }

    /* Loop until receiving all streams connected msgs or until receive times out */
    all_connected_count = 0;
    do {
        do {
            memcpy(&read_set, &init_read_set, sizeof(fd_set));
            timeout.tv_sec = test->settings->udp_connect_retry_timeout;
            timeout.tv_usec = 0;
            result = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
        } while (result < 0 && errno == EINTR);

        if (result < 0) {
            i_errno = IESELECT;
            return -1;
        }
        /* if recive timed out assume no more acks from the client */
        if (result == 0) {
            if (test->debug) {
                iperf_printf(test, "Receiving all streams connected msg timed out\n");
            }
            break; /* from waiting to all connected loop */
        }

        /* discard old connect replies */
        reply_to_discarded_connect_msg = (out_sa_peer == NULL) ? 0 : 1;
        if (iperf_udp_discard_old_connect_messages(test, &read_set, reply_to_discarded_connect_msg) > 0) { 
            continue; /* try to make sure all other messages are discurded before handling all connected msgs */
        }

        if (FD_ISSET(control_socket, &read_set)) { /* desired reply is available */ 
            sa_peer_len = sizeof(sa_peer);
            if ((sz = recvfrom(control_socket, &buf, sizeof(buf), 0, (struct sockaddr *) &sa_peer, &sa_peer_len)) < 0) {
                i_errno = IESTREAMACCEPT;
                return -1;
            }

            /* Ensure this is all connections available ack */
            if (buf == msg_type) {
                all_connected_count++;
                /* save mes information of first connect msg */
                if (all_connected_count == 1 && out_sa_peer != NULL) {
                    memcpy(out_sa_peer, &sa_peer, sa_peer_len);
                    *out_sa_peer_len = sa_peer_len;
                }
            } else if (test->debug_level >= DEBUG_LEVEL_ERROR) {
                iperf_printf(test, "Expected stream connected msg/reply of type x%x but received a message type x%x\n", msg_type, buf);
            }
        }
    } while (all_connected_count < test->settings->udp_connect_retries);

    if (test->debug) {
        iperf_printf(test, "Received %d (out of %d) all streams connected msgs/replies\n", all_connected_count, test->settings->udp_connect_retries);
    }

    return all_connected_count;
}


/*
 * iperf_udp_listen
 *
 * Start up a listener for UDP stream connections.  Unlike for TCP,
 * there is no listen(2) for UDP.  This socket will however accept
 * a UDP datagram from a client (indicating the client's presence).
 */
int
iperf_udp_listen(struct iperf_test *test)
{
    int s;

    if ((s = netannounce(test->settings->domain, Pudp, test->bind_address, test->bind_dev, test->server_port)) < 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    /*
     * The caller will put this value into test->prot_listener.
     */
    return s;
}


/*
 * iperf_udp_create_socket
 *
 * Create and bind local socket for UDP stream listener.
 */
int
iperf_udp_create_socket(struct iperf_test *test)
{
    int s;
#ifdef SO_RCVTIMEO
    struct timeval tv;
#endif
    int rc;

    /* Create and bind our local socket. */
    if ((s = netdial(test->settings->domain, Pudp, test->bind_address, test->bind_dev, test->bind_port, test->server_hostname, test->server_port, -1)) < 0) {
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    /* Check and set socket buffer sizes */
    rc = iperf_udp_buffercheck(test, s);
    if (rc < 0)
	/* error */
	return rc;
    /*
     * If the socket buffer was too small, but it was the default
     * size, then try explicitly setting it to something larger.
     */
    if (rc > 0) {
	if (test->settings->socket_bufsize == 0) {
            char str[WARN_STR_LEN];
	    int bufsize = test->settings->blksize + UDP_BUFFER_EXTRA;
	    snprintf(str, sizeof(str), "Increasing socket buffer size to %d",
	             bufsize);
	    warning(str);
	    test->settings->socket_bufsize = bufsize;
	    rc = iperf_udp_buffercheck(test, s);
	    if (rc < 0)
		return rc;
	}
    }

#if defined(HAVE_SO_MAX_PACING_RATE)
    /* If socket pacing is available and not disabled, try it. */
    if (test->settings->fqrate) {
	/* Convert bits per second to bytes per second */
	unsigned int fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		printf("Setting fair-queue socket pacing to %u\n", fqrate);
	    }
	    if (setsockopt(s, SOL_SOCKET, SO_MAX_PACING_RATE, &fqrate, sizeof(fqrate)) < 0) {
		warning("Unable to set socket pacing");
	    }
	}
    }
#endif /* HAVE_SO_MAX_PACING_RATE */
    {
	unsigned int rate = test->settings->rate / 8;
	if (rate > 0) {
	    if (test->debug) {
		printf("Setting application pacing to %u\n", rate);
	    }
	}
    }

#ifdef SO_RCVTIMEO
    /* 30 sec timeout for a case when there is a network problem. */
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
#endif

    return s;
}


/*
 * iperf_udp_discard_old_connect_messages
 *
 * Read left over connect requests or replies of  streams and throw them away.
 */
int
iperf_udp_discard_old_connect_messages(struct iperf_test *test, fd_set *read_set, int send_connect_reply_flag) {
    int s;
    struct iperf_stream *sp;
    unsigned int buf;
    int discarded_count = 0;

    if (test->protocol->id == Pudp &&  test->settings->udp_connect_retries > 1) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            s = sp->socket;
            if (FD_ISSET(s, read_set)) {
                discarded_count++;
                recv(s, &buf, sizeof(buf), 0);
                if (test->debug_level >= DEBUG_LEVEL_INFO) {
                    iperf_printf(test, "Discarded connect message from socket=%d - message_id=x%x,send_connect_reply_flag=%d\n", s, buf, send_connect_reply_flag);
                }

                /* Send reply to the repeated request as previous reply may not have been received */
                if (send_connect_reply_flag && buf == UDP_CONNECT_MSG) {
                    if (test->debug_level >= DEBUG_LEVEL_INFO) {
                        iperf_printf(test, "Send reply to the discarded late arrived connect msg from socket=%d - message_id=x%x\n", s, buf);
                    }

                    if (iperf_udp_send_connect_msg(test, s, UDP_CONNECT_REPLY, 0) < 0) {
                        return -1;
                    }
                }

                FD_CLR(s, read_set);
            }
        }
    }

    return discarded_count;
}


/*
 * iperf_udp_connect
 *
 * "Connect" to a UDP stream listener.
 */
int
iperf_udp_connect(struct iperf_test *test)
{
    int s, sz, total_sz, result, ret;
    int i, max_len_wait_for_reply;
    unsigned int buf;
    fd_set read_set, init_read_set;
    struct timeval timeout;
    struct iperf_stream *sp;
    int max_fd;

    if ((s = iperf_udp_create_socket(test)) < 0) {
        return s;
    }

    ret = -1; /* default return - failure */

    if (test->settings->udp_connect_retries < 2) {
        max_fd = 0;
    } else {
        /* receive only from test streams and not from other sockets */
        FD_ZERO(&init_read_set);
        FD_SET(s, &init_read_set);
        max_fd = s;
        SLIST_FOREACH(sp, &test->streams, streams) {
            FD_SET(sp->socket, &init_read_set);
            if (sp->socket > max_fd)
                max_fd = sp->socket;
        }
    }

    /*
     * Write a datagram to the UDP stream to let the server know we're here.
     * The server learns our address by obtaining its peer's address.
     */
    for (i=0; i < test->settings->udp_connect_retries  && ret < 0; i++) {
        if (test->debug_level >= DEBUG_LEVEL_INFO) {
            iperf_printf(test, "Sending Connect message x%x to Socket %d - retry number %d\n", UDP_CONNECT_MSG, s, i);
        }

        iperf_udp_send_connect_msg(test, s, UDP_CONNECT_MSG, 0);

        /*
        * Wait until the server replies back to us with the "accept" response, or timeout.
        */

        /* this functionality is applicable only when the server handles retries */
        if (test->settings->udp_connect_retries > 1) {
            do { /* get reply for this connection request */
                do {
                    memcpy(&read_set, &init_read_set, sizeof(fd_set));
                    timeout.tv_sec = test->settings->udp_connect_retry_timeout; /* Wait for server's ack with time out */
                    timeout.tv_usec = 0;
                    result = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
                } while (result < 0 && errno == EINTR);

                if (result < 0) {
                    i_errno = IESELECT;
                    return -1;
                } else if (result > 0) { /* some input received */
                    iperf_udp_discard_old_connect_messages(test, &read_set, 0); /* discurd prev streams connect responses */
                    if (FD_ISSET(s, &read_set)) { /* reply is available for the connection request */ 
                        break; /* from waiting for this connect reply */
                    }
                } else { /* result == 0 - select timed out*/
                    if (test->debug) {
                        iperf_printf(test, "Receiving server's connection ack for socket %d timed out after connect retry %d, errno=%s\n", s, i + 1, strerror(errno));
                    }    
                }
            } while(result > 0);

            if (result == 0) { /* on timed out select() - next connnect retry */
                continue; /* connect retry for loop */
            }
        }

        /* get the connect reply from the server */
        total_sz = 0;
        max_len_wait_for_reply = sizeof(buf);
        if (test->reverse && test->settings->udp_connect_retries < 2) {
            /* In reverse mode allow few packets to have the "accept" response - to handle out of order packets */
            max_len_wait_for_reply += MAX_REVERSE_OUT_OF_ORDER_PACKETS * test->settings->blksize;
        }

        do { 
            if ((sz = recv(s, &buf, sizeof(buf), 0)) < 0) {
                i_errno = IESTREAMREAD;
                return -1;
            }
            if (test->debug) {
                iperf_printf(test, "Connect reply received for Socket %d, sz=%d, msg_id=x%x, total_sz=%d, max_len_wait_for_reply=%d\n", s, sz, buf, total_sz, max_len_wait_for_reply);
            }
            total_sz += sz;
        } while (buf != UDP_CONNECT_REPLY && buf != LEGACY_UDP_CONNECT_REPLY && total_sz < max_len_wait_for_reply);

        /* Only receiving connect reply is allowed in this state */
        if (buf != UDP_CONNECT_REPLY && buf != LEGACY_UDP_CONNECT_REPLY) {
            i_errno = IESTREAMREAD;
            return -1;
        }

        ret = s; // Connection is successful

    } /* connect retry loop */

    if (ret < 0) {
        i_errno = IESTREAMREAD;
    }

    return ret;
}


/* iperf_udp_send_all_streams_connected_msgs
 *
 * Send to the server that all streams connected and serevrs' replies accepted by the client
 */
int
iperf_udp_send_all_streams_connected_msgs(struct iperf_test *test)
{
    int s;
    unsigned int rc;

    rc = 0;
    if (test->settings->udp_connect_retries > 1) {
        if ((s = iperf_udp_create_socket(test)) < 0) {
            return s;
        }

        if (test->debug_level >= DEBUG_LEVEL_INFO) {
            iperf_printf(test, "Sending all stream connected messages to socket %d\n", s);
        }

        /* send all streams connected message */
        rc = iperf_udp_send_connect_msg(test, s, UDP_ALL_STREAMS_CONNECTED_MSG, 1);

        /* receive all replies to the stream connected message */
        if (rc == 0) {
            if (iperf_udp_acceppt_all_streams_connected_msgs(test, UDP_ALL_STREAMS_CONNECTED_REPLY, s, NULL, NULL) == 0) {
                i_errno = IESTREAMCNCTEDREPLY;
                rc = -1;
            }
        }

        close(s);
    }

    return rc;
}


/* iperf_udp_init
 *
 * initializer for UDP streams in TEST_START
 */
int
iperf_udp_init(struct iperf_test *test)
{
    return 0;
}
