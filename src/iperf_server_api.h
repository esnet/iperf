#ifndef __IPERF_SERVER_API_H
#define __IPERF_SERVER_API_H

#include "iperf.h"

int iperf_run_server(struct iperf_test *);

int iperf_server_listen(struct iperf_test *);

int iperf_acept(struct iperf_test *);

int iperf_handle_message_server(struct iperf_test *);

void iperf_test_reset(struct iperf_test *);

#endif
