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

#ifdef HAVE_QUIC_NGTCP2

#ifndef __IPERF_QUIC_H
#define __IPERF_QUIC_H

#define IPERF_ALPN_STR "IPERF3-QUIC"

#define IPERF_QUIC_VERSION NGTCP2_PROTO_VER_V1

#define IPERF_QUIC_DEFAULT_MAX_IDLE_TIMEOUT 10000 // 10 seconds

#define IPERF_QUIC_MAX_TX_PAYLOAD_SIZE (1024 * 60) //TBD: should be Something else? (current value is from tests)

#define IPERF_QUIC_DEFAULT_MAX_RECV_UDP_PAYLOAD_SIZE NGTCP2_DEFAULT_MAX_RECV_UDP_PAYLOAD_SIZE //TBD: is this ok?

#define IPERF_QUIC_MAX_STREAM_WINDOWS (3 * IPERF_QUIC_MAX_TX_PAYLOAD_SIZE) //TBD: what is a good value?

/**
 * iperf_quic_recv -- receives the client data for QUIC
 *
 * returns state of packet received
 *
 */
int iperf_quic_recv(struct iperf_stream *sp);

/**
 * iperf_quic_send -- sends the client data for QUIC
 *
 * returns: bytes sent
 *
 */
int iperf_quic_send(struct iperf_stream *sp);

int iperf_quic_init(struct iperf_test *test);

int iperf_quic_connection_init(struct iperf_stream *sp);

void iperf_quic_delete_connection(struct iperf_quic_conn_data *quic_conn_data);

#endif

#endif /* HAVE_QUIC_NGTCP2 */
