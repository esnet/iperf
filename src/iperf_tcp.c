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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/select.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_tcp.h"
#include "iperf_error.h"
#include "net.h"


/* iperf_tcp_recv
 *
 * receives the data for TCP
 */
int
iperf_tcp_recv(struct iperf_stream *sp)
{
    int result = 0;
    int size = sp->settings->blksize;

    result = Nread(sp->socket, sp->buffer, size, Ptcp);

    if (result < 0) {
        return (-1);
    }

    sp->result->bytes_received += result;
    sp->result->bytes_received_this_interval += result;

    return (result);
}


/* iperf_tcp_send 
 *
 * sends the data for TCP
 */
int
iperf_tcp_send(struct iperf_stream *sp)
{
    int result;
    int size = sp->settings->blksize;

    result = Nwrite(sp->socket, sp->buffer, size, Ptcp);

    if (result < 0) {
        return (-1);
    }

    sp->result->bytes_sent += result;
    sp->result->bytes_sent_this_interval += result;

    return (result);
}


/* iperf_tcp_accept
 *
 * accept a new TCP stream connection
 */
int
iperf_tcp_accept(struct iperf_test * test)
{
    int     s;
    int     rbuf = ACCESS_DENIED;
    char    cookie[COOKIE_SIZE];
    socklen_t len;
    struct sockaddr_in addr;

    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IERECVCOOKIE;
        return (-1);
    }

    if (strcmp(test->cookie, cookie) != 0) {
        if (Nwrite(s, &rbuf, sizeof(char), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }
        close(s);
    }

    return (s);
}


/* iperf_tcp_listen
 *
 * start up a listener for TCP stream connections
 */
int
iperf_tcp_listen(struct iperf_test *test)
{
    int s, opt;
    struct addrinfo hints, *res;
    char portstr[6];
    s = test->listener;

    if (test->no_delay || test->settings->mss || test->settings->socket_bufsize) {
        FD_CLR(s, &test->read_set);
        close(s);
        if ((s = socket(test->settings->domain, SOCK_STREAM, 0)) < 0) {
            i_errno = IESTREAMLISTEN;
            return (-1);
        }
        if (test->no_delay) {
            opt = 1;
            if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
                i_errno = IESETNODELAY;
                return (-1);
            }
        }
        // XXX: Setting MSS is very buggy!
        if ((opt = test->settings->mss)) {
            if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
                i_errno = IESETMSS;
                return (-1);
            }
        }
        if ((opt = test->settings->socket_bufsize)) {
            if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0) {
                i_errno = IESETBUF;
                return (-1);
            }
            if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
                i_errno = IESETBUF;
                return (-1);
            }
        }
        opt = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            i_errno = IEREUSEADDR;
            return (-1);
        }

        snprintf(portstr, 6, "%d", test->server_port);
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = test->settings->domain;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        // XXX: Check getaddrinfo for errors!
        if (getaddrinfo(test->bind_address, portstr, &hints, &res) != 0) {
            i_errno = IESTREAMLISTEN;
            return (-1);
        }

        if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
            close(s);
            i_errno = IESTREAMLISTEN;
            return (-1);
        }

        freeaddrinfo(res);

        if (listen(s, 5) < 0) {
            i_errno = IESTREAMLISTEN;
            return (-1);
        }

        test->listener = s;
    }
    
    return (s);
}


/* iperf_tcp_connect
 *
 * connect to a TCP stream listener
 */
int
iperf_tcp_connect(struct iperf_test *test)
{
    int s, opt;
    struct addrinfo hints, *res;
    char portstr[6];

    if ((s = socket(test->settings->domain, SOCK_STREAM, 0)) < 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    if (test->bind_address) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = test->settings->domain;
        hints.ai_socktype = SOCK_STREAM;
        // XXX: Check getaddrinfo for errors!
        if (getaddrinfo(test->bind_address, NULL, &hints, &res) != 0) {
            i_errno = IESTREAMCONNECT;
            return (-1);
        }

        if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
            i_errno = IESTREAMCONNECT;
            return (-1);
        }

        freeaddrinfo(res);
    }

    /* Set socket options */
    if (test->no_delay) {
        opt = 1;
        if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
            i_errno = IESETNODELAY;
            return (-1);
        }
    }
    if ((opt = test->settings->mss)) {
        if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
            i_errno = IESETMSS;
            return (-1);
        }
    }
    if ((opt = test->settings->socket_bufsize)) {
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0) {
            i_errno = IESETBUF;
            return (-1);
        }
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
            i_errno = IESETBUF;
            return (-1);
        }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = test->settings->domain;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, 6, "%d", test->server_port);
    // XXX: Check getaddrinfo for errors!
    if (getaddrinfo(test->server_hostname, portstr, &hints, &res) != 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    if (connect(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0 && errno != EINPROGRESS) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    freeaddrinfo(res);

    /* Send cookie for verification */
    if (Nwrite(s, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IESENDCOOKIE;
        return (-1);
    }

    return (s);
}

