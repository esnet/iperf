/*
 * Copyright (c) 2009-2014, The Regents of the University of California,
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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/select.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_sctp.h"
#include "net.h"



/* iperf_sctp_recv
 *
 * receives the data for SCTP
 */
int
iperf_sctp_recv(struct iperf_stream *sp)
{
    int r;

    r = Nread(sp->socket, sp->buffer, sp->settings->blksize, Psctp);
    if (r < 0)
        return r;

    sp->result->bytes_received += r;
    sp->result->bytes_received_this_interval += r;

    return r;
}


/* iperf_sctp_send 
 *
 * sends the data for SCTP
 */
int
iperf_sctp_send(struct iperf_stream *sp)
{
    int r;

    r = Nwrite(sp->socket, sp->buffer, sp->settings->blksize, Psctp);
    if (r < 0)
        return r;    

    sp->result->bytes_sent += r;
    sp->result->bytes_sent_this_interval += r;

    return r;
}



/* iperf_sctp_accept
 *
 * accept a new SCTP stream connection
 */
int
iperf_sctp_accept(struct iperf_test * test)
{
    int     s;
    signed char rbuf = ACCESS_DENIED;
    char    cookie[COOKIE_SIZE];
    socklen_t len;
    struct sockaddr_storage addr;

    len = sizeof(addr);
    s = accept(test->listener, (struct sockaddr *) &addr, &len);
    if (s < 0) {
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    if (Nread(s, cookie, COOKIE_SIZE, Psctp) < 0) {
        i_errno = IERECVCOOKIE;
        return -1;
    }

    if (strcmp(test->cookie, cookie) != 0) {
        if (Nwrite(s, (char*) &rbuf, sizeof(rbuf), Psctp) < 0) {
            i_errno = IESENDMESSAGE;
            return -1;
        }
        close(s);
    }

    return s;
}


/* iperf_sctp_listen
 *
 * start up a listener for SCTP stream connections
 */
int
iperf_sctp_listen(struct iperf_test *test)
{
    struct addrinfo hints, *res;
    char portstr[6];
    int s, opt;

    close(test->listener);
   
    snprintf(portstr, 6, "%d", test->server_port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (test->settings->domain == AF_UNSPEC ? AF_INET6 : test->settings->domain);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(test->bind_address, portstr, &hints, &res) != 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    if ((s = socket(res->ai_family, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
        freeaddrinfo(res);
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    if (test->settings->domain == AF_UNSPEC || test->settings->domain == AF_INET6) {
        if (test->settings->domain == AF_UNSPEC)
            opt = 0;
        else if (test->settings->domain == AF_INET6)
            opt = 1;
        if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, 
		       (char *) &opt, sizeof(opt)) < 0) {
	    close(s);
	    freeaddrinfo(res);
	    i_errno = IEPROTOCOL;
	    return -1;
	}
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(s);
        freeaddrinfo(res);
        i_errno = IEREUSEADDR;
        return -1;
    }

    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        close(s);
        freeaddrinfo(res);
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    freeaddrinfo(res);

    if (listen(s, 5) < 0) {
        i_errno = IESTREAMLISTEN;
        return -1;
    }

    test->listener = s;
  
    return s;
}


/* iperf_sctp_connect
 *
 * connect to a SCTP stream listener
 */
int
iperf_sctp_connect(struct iperf_test *test)
{
    int s, opt;
    char portstr[6];
    struct addrinfo hints, *local_res, *server_res;

    if (test->bind_address) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = test->settings->domain;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(test->bind_address, NULL, &hints, &local_res) != 0) {
            i_errno = IESTREAMCONNECT;
            return -1;
        }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = test->settings->domain;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", test->server_port);
    if (getaddrinfo(test->server_hostname, portstr, &hints, &server_res) != 0) {
	if (test->bind_address)
	    freeaddrinfo(local_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }

    s = socket(server_res->ai_family, SOCK_STREAM, IPPROTO_SCTP);
    if (s < 0) {
	if (test->bind_address)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }

   
    if (connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) {
	close(s);
	freeaddrinfo(server_res);
        i_errno = IESTREAMCONNECT;
        return -1;
    }
    freeaddrinfo(server_res);

    /* Send cookie for verification */
    if (Nwrite(s, test->cookie, COOKIE_SIZE, Psctp) < 0) {
	close(s);
        i_errno = IESENDCOOKIE;
        return -1;
    }

    opt = 0;
    if(setsockopt(s, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS, &opt, sizeof(opt)) < 0) {
        close(s);
        freeaddrinfo(server_res);
        i_errno = IESETSCTPDISABLEFRAG;
        return -1;
    }

    return s;
}



int
iperf_sctp_init(struct iperf_test *test)
{
    return 0;
}

