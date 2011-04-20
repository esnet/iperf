/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_SERVER_API_H
#define __IPERF_SERVER_API_H

#include "iperf.h"

int iperf_run_server(struct iperf_test *);

int iperf_server_listen(struct iperf_test *);

int iperf_accept(struct iperf_test *);

int iperf_handle_message_server(struct iperf_test *);

void iperf_test_reset(struct iperf_test *);

#endif

