
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
#include "iperf_util.h"
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
    int result = 0;
    int size = sp->settings->blksize;

    if (!sp->buffer) {
        fprintf(stderr, "iperf_tcp_recv: receive buffer not allocated\n");
        return -1;
    }
#ifdef USE_RECV
	/*
	 * NOTE: Nwrite/Nread seems to be 10-15% faster than send/recv for
	 * localhost on OSX. More testing needed on other OSes to be sure.
	 */
    do {
        result = recv(sp->socket, sp->buffer, size, MSG_WAITALL);
    } while (result == -1 && errno == EINTR);
#else
    result = Nread(sp->socket, sp->buffer, size, Ptcp);
#endif
    if (result == -1) {
        perror("Read error");
        return -1;
    }
    sp->result->bytes_received += result;
    sp->result->bytes_received_this_interval += result;

    return result;
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
    int result;
    int size = sp->settings->blksize;

    if (!sp->buffer) {
        fprintf(stderr, "iperf_tcp_send: transmit buffer not allocated\n");
        return -1;
    }

#ifdef USE_SEND
    result = send(sp->socket, sp->buffer, size, 0);
#else
    result = Nwrite(sp->socket, sp->buffer, size, Ptcp);
#endif
    if (result < 0)
        perror("Write error");

    sp->result->bytes_sent += result;
    sp->result->bytes_sent_this_interval += result;

    return result;
}

/**************************************************************************/
struct iperf_stream *
iperf_new_tcp_stream(struct iperf_test * testp)
{
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) iperf_new_stream(testp);
    if (!sp) {
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
    int peersock;
    struct iperf_stream *sp;

    len = sizeof(addr);
    peersock = accept(test->prot_listener, (struct sockaddr *) & addr, &len);
    if (peersock < 0) {
        // XXX: Needs to implement better error handling
        printf("Error in accept(): %s\n", strerror(errno));
        return -1;
    }

    // XXX: Nonblocking off. OKAY since we use select.
    // setnonblocking(peersock);

    // XXX: This doesn't fit our API model!
    sp = test->new_stream(test);
    sp->socket = peersock;
    iperf_init_stream(sp, test);
    iperf_add_stream(test, sp);

    FD_SET(peersock, &test->read_set);  /* add new socket to master set */
    FD_SET(peersock, &test->write_set);
    test->max_fd = (test->max_fd < peersock) ? peersock : test->max_fd;

    connect_msg(sp);    /* print connect message */

    return 0;
}


