/*
 * iperf, Copyright (c) 2014-2017, The Regents of the University of
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
#ifndef __IPERF_UTIL_H
#define __IPERF_UTIL_H

#include "iperf_config.h"
#include "cjson.h"
#include <sys/select.h>
#include <stddef.h>

int readentropy(void *out, size_t outsize);

void fill_with_repeating_pattern(void *out, size_t outsize);

void make_cookie(char *);

int is_closed(int);

double timeval_to_double(struct timeval *tv);

int timeval_equals(struct timeval *tv0, struct timeval *tv1);

double timeval_diff(struct timeval *tv0, struct timeval *tv1);

void cpu_util(double pcpu[3]);

const char* get_system_info(void);

const char* get_optional_features(void);

cJSON* iperf_json_printf(const char *format, ...);

void iperf_dump_fdset(FILE *fp, char *str, int nfds, fd_set *fds);

#ifndef HAVE_DAEMON
extern int daemon(int nochdir, int noclose);
#endif /* HAVE_DAEMON */

#ifndef HAVE_GETLINE
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#endif /* HAVE_GETLINE */

#endif
