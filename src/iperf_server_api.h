#ifndef __IPERF_SERVER_API_H
#define __IPERF_SERVER_API_H

#include "iperf.h"

int param_received(struct iperf_stream *, struct param_exchange *);

void send_result_to_client(struct iperf_stream *);

void iperf_run_server(struct iperf_test *);

struct iperf_stream *find_stream_by_socket(struct iperf_test *, int);

int iperf_server_listen(struct iperf_test *);

#endif
