/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef        IPERF_SCTP_H
#define        IPERF_SCTP_H


#ifndef SCTP_DISABLE_FRAGMENTS
#define SCTP_DISABLE_FRAGMENTS  8
#endif

/**
 * iperf_sctp_accept -- accepts a new SCTP connection
 * on sctp_listener_socket for SCTP data and param/result
 * exchange messages
 *returns 0 on success
 *
 */
int iperf_sctp_accept(struct iperf_test *);

/**
 * iperf_sctp_recv -- receives the data for sctp
 * and the Param/result message exchange
 *returns state of packet received
 *
 */
int iperf_sctp_recv(struct iperf_stream *);


/**
 * iperf_sctp_send -- sends the client data for sctp
 * and the  Param/result message exchanges
 * returns: bytes sent
 *
 */
int iperf_sctp_send(struct iperf_stream *);


int iperf_sctp_listen(struct iperf_test *);

int iperf_sctp_connect(struct iperf_test *);

int iperf_sctp_init(struct iperf_test *test);

#endif
