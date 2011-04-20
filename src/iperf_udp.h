/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_UDP_H
#define __IPERF_UDP_H


/**
 * iperf_udp_recv -- receives the client data for UDP
 *
 *returns state of packet received
 *
 */
int iperf_udp_recv(struct iperf_stream *);

/**
 * iperf_udp_send -- sends the client data for UDP
 *
 * returns: bytes sent
 *
 */
int iperf_udp_send(struct iperf_stream *);


/**
 * iperf_udp_accept -- accepts a new UDP connection
 * on udp_listener_socket
 *returns 0 on success
 *
 */
int iperf_udp_accept(struct iperf_test *);


int iperf_udp_listen(struct iperf_test *);

int iperf_udp_connect(struct iperf_test *);

int iperf_udp_init(struct iperf_test *);


#endif

