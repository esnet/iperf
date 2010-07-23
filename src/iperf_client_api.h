
#ifndef __IPERF_CLIENT_API_H
#define __IPERF_CLIENT_API_H

#include "iperf.h"

int iperf_run_client(struct iperf_test *);

int iperf_connect(struct iperf_test *);

int iperf_create_streams(struct iperf_test *);

int iperf_handle_message_client(struct iperf_test *);

int iperf_client_end(struct iperf_test *);

#endif

