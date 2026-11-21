/*
 * iperf, Copyright (c) 2017-2020, The Regents of the University of
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


#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"

#include "version.h"

#include "units.h"

static int
check_udp_interval(struct iperf_test *test, int64_t packets, int64_t errors,
                   int64_t outoforder, int64_t expected_packets,
                   int64_t expected_errors, int64_t expected_outoforder)
{
    struct iperf_stream *sp = SLIST_FIRST(&test->streams);
    struct iperf_interval_results *irp;

    sp->packet_count = packets;
    sp->cnt_error = errors;
    sp->outoforder_packets = outoforder;
    iperf_stats_callback(test);
    irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);
    if (irp == NULL)
        return 1;
    if (irp->interval_packet_count != expected_packets ||
        irp->interval_cnt_error != expected_errors ||
        irp->interval_outoforder_packets != expected_outoforder ||
        irp->packet_count != packets || irp->cnt_error != errors ||
        irp->outoforder_packets != outoforder || irp->omitted != test->omitting) {
        fprintf(stderr, "UDP interval (omit=%d, omitting=%d): expected "
                "packets/errors/outoforder=%" PRId64 "/%" PRId64 "/%" PRId64
                ", got %" PRId64 "/%" PRId64 "/%" PRId64 "\n",
                test->omit, test->omitting, expected_packets, expected_errors,
                expected_outoforder, irp->interval_packet_count,
                irp->interval_cnt_error, irp->interval_outoforder_packets);
        return 1;
    }
    return 0;
}

/* Stable counters exercise the actual reset/stats callbacks without workers. */
static int
test_udp_interval_baselines(int omit, int prior_interval,
                            int64_t expected_packets, int64_t expected_errors,
                            int64_t expected_outoforder)
{
    struct iperf_test *test;
    struct iperf_stream stream = {0};
    struct iperf_stream_result result = {0};
    struct iperf_interval_results *irp;
    int ret = 0;

    test = iperf_new_test();
    if (test == NULL)
        return 1;
    if (iperf_defaults(test) < 0 || set_protocol(test, Pudp) < 0) {
        iperf_free_test(test);
        return 1;
    }
    test->role = 'c';
    test->sender_has_retransmits = 0;
    test->omit = omit;
    test->omitting = omit;
    stream.test = test;
    stream.settings = test->settings;
    stream.result = &result;
    TAILQ_INIT(&result.interval_results);
    SLIST_INSERT_HEAD(&test->streams, &stream, streams);
    if (iperf_time_now(&result.start_time) < 0) {
        ret = 1;
        goto cleanup;
    }
    result.end_time = result.start_time;

    if (prior_interval) {
        ret += check_udp_interval(test, 70, 8, 3, 70, 8, 3);
        ret += check_udp_interval(test, 75, 7, 4, 5, -1, 1);
    }
    test->omitting = 0;
    if (omit) {
        stream.packet_count = 100;
        stream.cnt_error = 13;
        stream.outoforder_packets = 5;
        iperf_reset_stats(test);
    } else {
        /* These baselines must be ignored when omission is not configured. */
        stream.omitted_packet_count = 100;
        stream.omitted_cnt_error = 13;
        stream.omitted_outoforder_packets = 5;
    }
    /* Late packets can reduce the signed loss estimate across the boundary. */
    ret += check_udp_interval(test, 120, 11, 9, expected_packets,
                              expected_errors, expected_outoforder);
    ret += check_udp_interval(test, 135, 12, 10, 15, 1, 1);

cleanup:
    while ((irp = TAILQ_FIRST(&result.interval_results)) != NULL) {
        TAILQ_REMOVE(&result.interval_results, irp, irlistentries);
        free(irp);
    }
    SLIST_REMOVE_HEAD(&test->streams, streams);
    iperf_free_test(test);
    return ret;
}

int test_iperf_set_test_bind_port(struct iperf_test *test)
{
    int port;
    port = iperf_get_test_bind_port(test);
    iperf_set_test_bind_port(test, 5202);
    port = iperf_get_test_bind_port(test);
    assert(port == 5202);
    return 0;
}

int test_iperf_set_mss(struct iperf_test *test)
{
    int mss = iperf_get_test_mss(test);
    iperf_set_test_mss(test, 535);
    mss = iperf_get_test_mss(test);
    assert(mss == 535);
    return 0;
}

int
main(int argc, char **argv)
{
    const char *ver;
    struct iperf_test *test;
    int sint, gint, ret;

    /* Run all cases, including when assertions are compiled out. */
    ret = test_udp_interval_baselines(1, 1, 20, -2, 4);
    ret += test_udp_interval_baselines(1, 0, 20, -2, 4);
    ret += test_udp_interval_baselines(0, 1, 45, 4, 5);
    ret += test_udp_interval_baselines(0, 0, 120, 11, 9);
    if (ret != 0)
        return 1;

    ver = iperf_get_iperf_version();
    assert(strcmp(ver, IPERF_VERSION) == 0);

    test = iperf_new_test();
    assert(test != NULL);

    iperf_defaults(test);

    sint = 10;
    iperf_set_test_connect_timeout(test, sint);
    gint = iperf_get_test_connect_timeout(test);
    assert(sint == gint);

    ret = test_iperf_set_test_bind_port(test);

    ret += test_iperf_set_mss(test);

    iperf_free_test(test);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}
