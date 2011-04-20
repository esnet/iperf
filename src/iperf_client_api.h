/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_CLIENT_API_H
#define __IPERF_CLIENT_API_H

#include "iperf.h"

int iperf_run_client(struct iperf_test *);

int iperf_connect(struct iperf_test *);

int iperf_create_streams(struct iperf_test *);

int iperf_handle_message_client(struct iperf_test *);

int iperf_client_end(struct iperf_test *);

#endif

