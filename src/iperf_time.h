/*
 * iperf, Copyright (c) 2014-2018, The Regents of the University of
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
#ifndef __IPERF_TIME_H
#define __IPERF_TIME_H

#include <stdint.h>

struct iperf_time {
    uint32_t secs;
    uint32_t usecs;
};

int iperf_time_now(struct iperf_time *time1);

void iperf_time_add_usecs(struct iperf_time *time1, uint64_t usecs);

int iperf_time_compare(struct iperf_time *time1, struct iperf_time *time2);

int iperf_time_diff(struct iperf_time *time1, struct iperf_time *time2, struct iperf_time *diff);

uint64_t iperf_time_in_usecs(struct iperf_time *time);

double iperf_time_in_secs(struct iperf_time *time);

#endif
