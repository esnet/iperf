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
#include "iperf_config.h"

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
#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/select.h>
#if defined(HAVE_UDP_SEGMENT) || defined(HAVE_UDP_GRO)
#include <linux/udp.h>
#endif

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_udp.h"
#include "timer.h"
#include "net.h"
#include "cjson.h"

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
    struct iperf_test *test = sp->test;
    int sock_opt = 0;
    int       dgram_sz;
    int       buf_sz;
    char      *dgram_buf;
    char      *dgram_buf_end;
    const int min_pkt_size = sizeof(uint32_t) * 3; /* sec + usec + pcount (32-bit) */

#if defined(HAVE_MSG_TRUNC)
    // UDP recv() with MSG_TRUNC reads only the size bytes, but return the length of the full packet
    if (sp->test->settings->skip_rx_copy) {
        sock_opt = MSG_TRUNC;
        size = sizeof(sec) + sizeof(usec) + sizeof(pcount);
    }
#endif /* HAVE_MSG_TRUNC */

    /* Configure loop parameters based on GRO availability */
    if (sp->test->settings->gro) {
	size = sp->test->settings->gro_bf_size;
	r = Nread_gro(sp->socket, sp->buffer, size, Pudp, &dgram_sz);
	/* Use negotiated block size for GRO segment stride to ensure correct parsing. */
	dgram_sz = sp->settings->blksize;
	buf_sz = r;
    } else {
	/* GRO disabled or unavailable - use normal UDP receive and single packet size */
	r = Nrecv_no_select(sp->socket, sp->buffer, size, Pudp, sock_opt);
	dgram_sz = sp->settings->blksize;
	buf_sz = r;
    }

    /*
     * If we got an error in the read, or if we didn't read anything
     * because the underlying read(2) got a EAGAIN, then skip packet
     * processing.
     */
    if (r <= 0)
        return r;

    /* Only count bytes received while we're in the correct state. */
    if (test->state == TEST_RUNNING) {

	/*
	 * For jitter computation below, it's important to know if this
	 * packet is the first packet received.
	 */
	if (sp->result->bytes_received == 0) {
	    first_packet = 1;
	}

	sp->result->bytes_received += r;
	sp->result->bytes_received_this_interval += r;

	if (sp->test->debug)
	    printf("received %d bytes of %d, total %" PRIu64 "\n", r, size, sp->result->bytes_received);

	/* Unified loop: processes single packet when GRO off, multiple when GRO on */
	dgram_buf = sp->buffer;
	dgram_buf_end = sp->buffer + buf_sz;

	while (buf_sz >= dgram_sz && dgram_buf + dgram_sz <= dgram_buf_end) {

	    /* Ensure we have enough bytes for the packet header */
	    if (buf_sz < min_pkt_size)
		break;

	    /* Extract packet headers */
	    if (sp->test->udp_counters_64bit) {
		/* Verify we have enough space for 64-bit counter */
		if (buf_sz < sizeof(uint32_t) * 2 + sizeof(uint64_t))
		    break;
		memcpy(&sec, dgram_buf, sizeof(sec));
		memcpy(&usec, dgram_buf+4, sizeof(usec));
		memcpy(&pcount, dgram_buf+8, sizeof(pcount));
		sec = ntohl(sec);
		usec = ntohl(usec);
		pcount = be64toh(pcount);
		sent_time.secs = sec;
		sent_time.usecs = usec;
	    } else {
		uint32_t pc;
		memcpy(&sec, dgram_buf, sizeof(sec));
		memcpy(&usec, dgram_buf+4, sizeof(usec));
		memcpy(&pc, dgram_buf+8, sizeof(pc));
		sec = ntohl(sec);
		usec = ntohl(usec);
		pcount = ntohl(pc);
		sent_time.secs = sec;
		sent_time.usecs = usec;
	    }

	    /* Loss/out-of-order accounting - now always executed */
	    if (pcount >= sp->packet_count + 1) {
		if (pcount > sp->packet_count + 1) {
		    sp->cnt_error += (pcount - 1) - sp->packet_count;
		}
		sp->packet_count = pcount;
	    } else {
		sp->outoforder_packets++;
		if (sp->cnt_error > 0)
		    sp->cnt_error--;
	    }

	    /* Jitter computation - now always executed */
	    iperf_time_now(&arrival_time);
	    iperf_time_diff(&arrival_time, &sent_time, &temp_time);
	    transit = iperf_time_in_secs(&temp_time);
	    if (first_packet)
		sp->prev_transit = transit;
	    d = transit - sp->prev_transit;
	    if (d < 0)
		d = -d;
	    sp->prev_transit = transit;
	    sp->jitter += (d - sp->jitter) / 16.0;
	    first_packet = 0;

	    dgram_buf += dgram_sz;
	    buf_sz -= dgram_sz;
	}
    }
    else {
	if (test->debug_level >= DEBUG_LEVEL_INFO)
	    printf("Late receive, state = %d\n", test->state);
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
    int       dgram_sz;
    int       buf_sz;
    int       cnt = 0;
    char      *dgram_buf;
    char      *dgram_buf_end;
    const int min_pkt_size = sizeof(uint32_t) * 3; /* sec + usec + pcount (32-bit) */

    /* Configure loop parameters based on GSO availability */
    if (sp->test->settings->gso) {
	dgram_sz = sp->test->settings->gso_dg_size;
	buf_sz = sp->test->settings->gso_bf_size;
	/* Validate GSO parameters */
	if (dgram_sz <= 0 || dgram_sz < min_pkt_size || dgram_sz > buf_sz) {
	    if (sp->test->debug_level >= DEBUG_LEVEL_INFO)
		printf("Invalid GSO dgram_sz %d for buf_sz %d, disabling GSO\n", dgram_sz, buf_sz);
	    dgram_sz = buf_sz = size;
	    sp->test->settings->gso = 0;  /* Disable GSO for safety */
	}
    } else {
	/* GSO disabled or unavailable - single packet */
	dgram_sz = buf_sz = size;
    }

    dgram_buf = sp->buffer;
    dgram_buf_end = sp->buffer + buf_sz;

    /* Unified loop: processes single packet when GSO off, multiple when GSO on */
    while (buf_sz > 0 && dgram_buf + dgram_sz <= dgram_buf_end) {
	cnt++;

	if (sp->test->debug)
	    printf("%d (%d) remaining %d\n", cnt, dgram_sz, buf_sz);

	/* Prevent buffer underflow */
	if (buf_sz < dgram_sz) {
	    if (sp->test->debug_level >= DEBUG_LEVEL_INFO)
		printf("Buffer underflow protection: buf_sz %d < dgram_sz %d\n", buf_sz, dgram_sz);
	    break;
	}

	iperf_time_now(&before);
	++sp->packet_count;

	if (sp->test->udp_counters_64bit) {
	    uint32_t  sec, usec;
	    uint64_t  pcount;

	    sec = htonl(before.secs);
	    usec = htonl(before.usecs);
	    pcount = htobe64(sp->packet_count);

	    memcpy(dgram_buf, &sec, sizeof(sec));
	    memcpy(dgram_buf+4, &usec, sizeof(usec));
	    memcpy(dgram_buf+8, &pcount, sizeof(pcount));
	} else {
	    uint32_t  sec, usec, pcount;

	    sec = htonl(before.secs);
	    usec = htonl(before.usecs);
	    pcount = htonl(sp->packet_count);

	    memcpy(dgram_buf, &sec, sizeof(sec));
	    memcpy(dgram_buf+4, &usec, sizeof(usec));
	    memcpy(dgram_buf+8, &pcount, sizeof(pcount));
	}

	dgram_buf += dgram_sz;
	buf_sz -= dgram_sz;
    }

    /* Warn if we didn't process all the buffer due to size mismatch */
    if (buf_sz > 0 && sp->test->debug_level >= DEBUG_LEVEL_INFO) {
	printf("GSO: %d bytes remaining unprocessed\n", buf_sz);
    }

    if (sp->test->settings->gso) {
        size = sp->test->settings->gso_bf_size;
        r = Nwrite_gso(sp->socket, sp->buffer, size, Pudp, sp->test->settings->gso_dg_size);
    } else {
        r = Nwrite(sp->socket, sp->buffer, size, Pudp);
    }

    if (r <= 0) {
        --sp->packet_count;     /* Don't count messages that no data was sent from them.
                                 * Allows "resending" a massage with the same numbering */
        if (r < 0) {
            if (r == NET_SOFTERROR && sp->test->debug_level >= DEBUG_LEVEL_INFO)
                printf("UDP send failed on NET_SOFTERROR. errno=%s\n", strerror(errno));
            return r;
        }
    }

    sp->result->bytes_sent += r;
    sp->result->bytes_sent_this_interval += r;

    if (sp->test->debug_level >=  DEBUG_LEVEL_DEBUG)
	printf("sent %d bytes of %d, total %" PRIu64 "\n", r, size, sp->result->bytes_sent);

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
    cJSON *sock_bufsize_item = cJSON_GetObjectItem(test->json_start, "sock_bufsize");
    if (sock_bufsize_item == NULL) {
    cJSON_AddNumberToObject(test->json_start, "sock_bufsize", test->settings->socket_bufsize);
    }

    cJSON *sndbuf_actual_item = cJSON_GetObjectItem(test->json_start, "sndbuf_actual");
    if (sndbuf_actual_item == NULL) {
	cJSON_AddNumberToObject(test->json_start, "sndbuf_actual", sndbuf_actual);
    }
        
    cJSON *rcvbuf_actual_item = cJSON_GetObjectItem(test->json_start, "rcvbuf_actual");
    if (rcvbuf_actual_item == NULL) {
	cJSON_AddNumberToObject(test->json_start, "rcvbuf_actual", rcvbuf_actual);
    }
    }

    return rc;
}

#ifdef HAVE_UDP_SEGMENT
int
iperf_udp_gso(struct iperf_test *test, int s)
{
    int rc;
    int gso = test->settings->gso_dg_size;

    rc = setsockopt(s, IPPROTO_UDP, UDP_SEGMENT, (char*) &gso, sizeof(gso));
    if (rc) {
	if (test->debug)
	    iperf_printf(test, "No GSO (%d)\n", rc);
        test->settings->gso = 0;
    } else {
	if (test->debug)
	    iperf_printf(test, "GSO (%d)\n", gso);
    }

    return rc;
}
#else
int
iperf_udp_gso(struct iperf_test *test, int s)
{
    /* GSO not supported on this platform */
    test->settings->gso = 0;
    return -1;
}
#endif

#ifdef HAVE_UDP_GRO
int
iperf_udp_gro(struct iperf_test *test, int s)
{
    int rc;
    int gro = 1;

    rc = setsockopt(s, IPPROTO_UDP, UDP_GRO, (char*) &gro, sizeof(gro));
    if (rc) {
	if (test->debug)
	    iperf_printf(test, "No GRO (%d)\n", rc);
        test->settings->gro = 0;
    } else {
	if (test->debug)
	    iperf_printf(test, "GRO\n");
    }

    return rc;
}
#else
int
iperf_udp_gro(struct iperf_test *test, int s)
{
    /* GRO not supported on this platform */
    test->settings->gro = 0;
    return -1;
}
#endif

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
    int	      rc;

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

    if (connect(s, (struct sockaddr *) &sa_peer, len) < 0) {
        i_errno = IESTREAMACCEPT;
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

    if (test->settings->gso)
        iperf_udp_gso(test, s);
    if (test->settings->gro)
        iperf_udp_gro(test, s);

#if defined(HAVE_SO_MAX_PACING_RATE)
    /* If socket pacing is specified, try it. */
    if (test->settings->fqrate) {
	/* Convert bits per second to bytes per second */
	uint64_t fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		printf("Setting fair-queue socket pacing to %"PRIu64"\n", fqrate);
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

    /*
     * Create a new "listening" socket to replace the one we were using before.
     */
    FD_CLR(test->prot_listener, &test->read_set); // No control messages from old listener
    test->prot_listener = netannounce(test->settings->domain, Pudp, test->bind_address, test->bind_dev, test->server_port);
    if (test->prot_listener < 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    FD_SET(test->prot_listener, &test->read_set);
    test->max_fd = (test->max_fd < test->prot_listener) ? test->prot_listener : test->max_fd;

    /* Let the client know we're ready "accept" another UDP "stream" */
    buf = UDP_CONNECT_REPLY;
    if (write(s, &buf, sizeof(buf)) < 0) {
        i_errno = IESTREAMWRITE;
        return -1;
    }

    return s;
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
 * iperf_udp_connect
 *
 * "Connect" to a UDP stream listener.
 */
int
iperf_udp_connect(struct iperf_test *test)
{
    int s, sz;
    unsigned int buf;
#ifdef SO_RCVTIMEO
    struct timeval tv;
#endif
    int rc;
    int i, max_len_wait_for_reply;

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

    if (test->settings->gso)
        iperf_udp_gso(test, s);
    if (test->settings->gro)
        iperf_udp_gro(test, s);

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
	uint64_t fqrate = test->settings->fqrate / 8;
	if (fqrate > 0) {
	    if (test->debug) {
		printf("Setting fair-queue socket pacing to %"PRIu64"\n", fqrate);
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

    /* Set common socket options */
    iperf_common_sockopts(test, s);

#ifdef SO_RCVTIMEO
    /* 30 sec timeout for a case when there is a network problem. */
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
#endif

    /*
     * Write a datagram to the UDP stream to let the server know we're here.
     * The server learns our address by obtaining its peer's address.
     */
    buf = UDP_CONNECT_MSG;
    if (test->debug) {
        printf("Sending Connect message to Socket %d\n", s);
    }
    if (write(s, &buf, sizeof(buf)) < 0) {
        // XXX: Should this be changed to IESTREAMCONNECT?
        i_errno = IESTREAMWRITE;
        return -1;
    }

    /*
     * Wait until the server replies back to us with the "accept" response.
     */
    i = 0;
    max_len_wait_for_reply = sizeof(buf);
    if (test->reverse) /* In reverse mode allow few packets to have the "accept" response - to handle out of order packets */
        max_len_wait_for_reply += MAX_REVERSE_OUT_OF_ORDER_PACKETS * test->settings->blksize;
    do {
        if ((sz = recv(s, &buf, sizeof(buf), 0)) < 0) {
            i_errno = IESTREAMREAD;
            return -1;
        }
        if (test->debug) {
            printf("Connect received for Socket %d, sz=%d, buf=%x, i=%d, max_len_wait_for_reply=%d\n", s, sz, buf, i, max_len_wait_for_reply);
        }
        i += sz;
    } while (buf != UDP_CONNECT_REPLY && buf != LEGACY_UDP_CONNECT_REPLY && i < max_len_wait_for_reply);

    if (buf != UDP_CONNECT_REPLY  && buf != LEGACY_UDP_CONNECT_REPLY) {
        i_errno = IESTREAMREAD;
        return -1;
    }

    return s;
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
