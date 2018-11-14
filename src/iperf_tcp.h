/*
 * iperf, Copyright (c) 2014, The Regents of the University of
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
int iperf_tcp_send(struct iperf_stream *) /* __attribute__((hot)) */;


int iperf_tcp_listen(struct iperf_test *);

int iperf_tcp_connect(struct iperf_test *);


#endif
