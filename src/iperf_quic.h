#ifndef __IPERF_QUIC_H
#define __IPERF_QUIC_H

#include "iperf.h"

struct iperf_quic_context;

int iperf_quic_init(struct iperf_test *test);
int iperf_quic_listen(struct iperf_test *test);
int iperf_quic_accept(struct iperf_test *test);
int iperf_quic_connect(struct iperf_test *test);
int iperf_quic_send(struct iperf_stream *sp);
int iperf_quic_recv(struct iperf_stream *sp);

int iperf_quic_attach_stream(struct iperf_test *test, struct iperf_stream *sp, int stream_id);
void iperf_quic_stream_cleanup(struct iperf_stream *sp);
void iperf_quic_close_listener(struct iperf_test *test);
void iperf_quic_reset_test(struct iperf_test *test);
void iperf_quic_free_test(struct iperf_test *test);
int iperf_quic_shutdown_complete(struct iperf_test *test);

#endif /* __IPERF_QUIC_H */
