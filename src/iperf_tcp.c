
/*
 * Copyright (c) 2009, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

// XXX: Surely we do not need all these headers!
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
#include "iperf_error.h"
#include "iperf_server_api.h"
#include "iperf_tcp.h"
#include "timer.h"
#include "net.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "locale.h"

// XXX: Does this belong here? We should probably declare this in iperf_api.c then put an extern in the header
jmp_buf   env;			/* to handle longjmp on signal */


/* iperf_tcp_recv
 *
 * receives the data for TCP
 */
int
iperf_tcp_recv(struct iperf_stream * sp)
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
iperf_tcp_send(struct iperf_stream * sp)
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


// XXX: This function is now deprecated
/**************************************************************************/
struct iperf_stream *
iperf_new_tcp_stream(struct iperf_test * testp)
{
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) iperf_new_stream(testp);
    if (!sp) {
        return (NULL);
    }
    sp->rcv = iperf_tcp_recv;	/* pointer to receive function */
    sp->snd = iperf_tcp_send;	/* pointer to send function */

    return sp;
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
    if ((s = accept(test->listener_tcp, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IERECVCOOKIE;
        return (-1);
    }

    if (strcmp(test->default_settings->cookie, cookie) != 0) {
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
    struct sockaddr_in sa;
    s = test->listener_tcp;

    if (test->no_delay || test->default_settings->mss) {
        FD_CLR(s, &test->read_set);
        close(s);
        if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            i_errno = IESTREAMLISTEN;
            return (-1);
        }
        if (test->no_delay) {
            opt = 1;
            if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
                i_errno = IESETNODELAY;
                return (-1);
            }
            printf("      TCP NODELAY: on\n");
        }
        // XXX: Setting MSS is very buggy!
        if ((opt = test->default_settings->mss)) {
            if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
                i_errno = IESETMSS;
                return (-1);
            }
            printf("      TCP MSS: %d\n", opt);
        }
        opt = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            i_errno = IEREUSEADDR;
            return (-1);
        }

        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        sa.sin_port = htons(test->server_port);

        if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
            close(s);
            i_errno = IESTREAMLISTEN;
            return (-1);
        }

        if (listen(s, 5) < 0) {
            i_errno = IESTREAMLISTEN;
            return (-1);
        }

        test->listener_tcp = s;
/*
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        FD_SET(test->listener_tcp, &test->read_set);
*/
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
    struct sockaddr_in sa;
    struct hostent *hent;

    if ((hent = gethostbyname(test->server_hostname)) == 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    memcpy(&sa.sin_addr.s_addr, hent->h_addr, sizeof(sa.sin_addr.s_addr));
    sa.sin_port = htons(test->server_port);

    /* Set TCP options */
    if (test->no_delay) {
        opt = 1;
        if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
            i_errno = IESETNODELAY;
            return (-1);
        }
    }
    if (opt = test->default_settings->mss) {
        if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &opt, sizeof(opt)) < 0) {
            i_errno = IESETMSS;
            return (-1);
        }
    }

    if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0 && errno != EINPROGRESS) {
        i_errno = IESTREAMCONNECT;
        return (-1);
    }

    /* Send cookie for verification */
    if (Nwrite(s, test->default_settings->cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IESENDCOOKIE;
        return (-1);
    }

    return (s);
}

