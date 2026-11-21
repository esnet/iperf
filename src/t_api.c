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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"

#include "version.h"

#include "units.h"

/* Exercise the real reset and stats callbacks without sockets or sleeps. */
static int
test_interval_times(int omit, int prior_interval)
{
    struct iperf_test *test;
    struct iperf_stream stream = {0};
    struct iperf_stream_result result = {0};
    struct iperf_interval_results *irp;
    struct iperf_time now, expected_start, elapsed, fixed_start;
    int ret = 0, sample;

    test = iperf_new_test();
    if (test == NULL)
        return 1;
    if (iperf_defaults(test) < 0 || set_protocol(test, Pudp) < 0) {
        iperf_free_test(test);
        return 1;
    }
    /* UDP avoids TCP socket-info collection in this timestamp-only fixture. */
    test->role = 'c';
    test->sender_has_retransmits = 0;
    stream.test = test;
    stream.settings = test->settings;
    stream.result = &result;
    TAILQ_INIT(&result.interval_results);
    SLIST_INSERT_HEAD(&test->streams, &stream, streams);

    if (iperf_time_now(&now) < 0 || now.secs < 2) {
        ret = 1;
        goto cleanup;
    }
    result.start_time = now;
    result.start_time.secs -= 2;
    result.start_time_fixed = fixed_start = result.start_time;
    result.end_time = result.start_time;
    result.end_time.secs++;
    if (prior_interval) {
        irp = calloc(1, sizeof(*irp));
        if (irp == NULL) {
            ret = 1;
            goto cleanup;
        }
        irp->interval_start_time = result.start_time;
        irp->interval_end_time = result.end_time;
        irp->interval_duration = 1.0;
        irp->omitted = omit;
        TAILQ_INSERT_TAIL(&result.interval_results, irp, irlistentries);
    }

    expected_start = prior_interval ? result.end_time : result.start_time;
    for (sample = 0; sample < 2; sample++) {
        if (omit) {
            test->omitting = 1;
            iperf_reset_stats(test);
            expected_start = result.start_time;
            if (iperf_time_compare(&result.end_time, &expected_start) != 0) {
                fprintf(stderr, "reset did not rebase interval end (prior=%d, sample=%d)\n",
                        prior_interval, sample);
                ret = 1;
            }
            /* Reset must not rewrite the preceding omitted snapshot. */
            if (prior_interval && sample == 0 &&
                (irp->interval_end_time.secs != now.secs - 1 ||
                 irp->interval_end_time.usecs != now.usecs || !irp->omitted)) {
                fprintf(stderr, "reset changed the preceding omitted interval\n");
                ret = 1;
            }
        }
        test->omitting = 0;
        iperf_stats_callback(test);
        /* Sampling replaces the previous node, so reacquire it each time. */
        irp = TAILQ_LAST(&result.interval_results, irlisthead);
        if (irp == NULL) {
            ret = 1;
            goto cleanup;
        }
        iperf_time_diff(&expected_start, &irp->interval_end_time, &elapsed);
        if (iperf_time_compare(&irp->interval_start_time, &expected_start) != 0 ||
            iperf_time_compare(&irp->interval_end_time, &result.end_time) != 0 ||
            iperf_time_compare(&result.start_time_fixed, &fixed_start) != 0 ||
            irp->interval_duration != (float) iperf_time_in_secs(&elapsed) ||
            irp->omitted) {
            fprintf(stderr, "incorrect interval times (omit=%d, prior=%d, sample=%d)\n",
                    omit, prior_interval, sample);
            ret = 1;
        }
        expected_start = result.end_time;
    }

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
    int sint, gint;

    /* Keep setup and checks active when assertions are compiled out. */
    if (test_interval_times(1, 1) != 0 ||
        test_interval_times(1, 0) != 0 ||
        test_interval_times(0, 0) != 0)
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

    int ret;
    ret = test_iperf_set_test_bind_port(test);

    ret += test_iperf_set_mss(test);

    iperf_free_test(test);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}
