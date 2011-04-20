/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef        IPERF_TCP_H
#define        IPERF_TCP_H


/**
 * iperf_tcp_accept -- accepts a new TCP connection
 * on tcp_listener_socket for TCP data and param/result
 * exchange messages
 *returns 0 on success
 *
 */
int iperf_tcp_accept(struct iperf_test *);

/**
 * iperf_tcp_recv -- receives the data for TCP
 * and the Param/result message exchange
 *returns state of packet received
 *
 */
int iperf_tcp_recv(struct iperf_stream *);


/**
 * iperf_tcp_send -- sends the client data for TCP
 * and the  Param/result message exchanges
 * returns: bytes sent
 *
 */
int iperf_tcp_send(struct iperf_stream *);


int iperf_tcp_listen(struct iperf_test *);

int iperf_tcp_connect(struct iperf_test *);


#endif

