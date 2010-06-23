#ifndef __IPERF_SERVER_API_H
#define __IPERF_SERVER_API_H

#include "iperf.h"

void send_result_to_client(struct iperf_stream *);

void iperf_run_server(struct iperf_test *);

int iperf_server_listen(struct iperf_test *);

#endif
