
/*
   Copyright (c) 2009, The Regents of the University of California, through
   Lawrence Berkeley National Laboratory (subject to receipt of any required
   approvals from the U.S. Dept. of Energy).  All rights reserved.
*/

#ifndef        IPERF_UDP_H
#define        IPERF_UDP_H


/**
 * iperf_udp_accept -- accepts a new UDP connection
 * on udp_listener_socket
 *returns 0 on success
 *
 */
int       iperf_udp_accept(struct iperf_test * test);


/**
 * iperf_udp_recv -- receives the client data for UDP
 *
 *returns state of packet received
 *
 */
int       iperf_udp_recv(struct iperf_stream * sp);

/**
 * iperf_udp_send -- sends the client data for UDP
 *
 * returns: bytes sent
 *
 */
int       iperf_udp_send(struct iperf_stream * sp);


/**
 * iperf_udp_accept -- accepts a new UDP connection
 * on udp_listener_socket
 *returns 0 on success
 *
 */
int       iperf_udp_accept(struct iperf_test * test);

struct iperf_stream *iperf_new_udp_stream(struct iperf_test * testp);


#endif  /* IPERF_UDP_H */

