/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_UTIL_H
#define __IPERF_UTIL_H

#include "cjson.h"

void make_cookie(char *);

int is_closed(int);

double timeval_to_double(struct timeval *tv);

int timeval_equals(struct timeval *tv0, struct timeval *tv1);

double timeval_diff(struct timeval *tv0, struct timeval *tv1);

int delay(int64_t ns);

void cpu_util(double pcpu[3]);

char* get_system_info(void);

cJSON* iperf_json_printf(const char *format, ...);

void iperf_dump_fdset(FILE *fp, char *str, int nfds, fd_set *fds);

#endif
