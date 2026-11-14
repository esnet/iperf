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
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "net.h"

#include "version.h"

#include "units.h"

static int
test_udp_send_rollback(int gso, int counters_64bit)
{
    struct iperf_test test = {0};
    struct iperf_settings settings = {0};
    struct iperf_stream stream = {0};
    struct iperf_stream_result result = {0};
    char buffer[128];
    int64_t initial_count = counters_64bit ? ((int64_t) 1 << 32) + 100 : 100;
    int datagrams = gso ? 4 : 1;
    int attempt, i, r, ret = 0;
    uint32_t sequence32;
    uint64_t sequence, expected;

    test.settings = &settings;
    test.udp_counters_64bit = counters_64bit;
    settings.blksize = 32;
    settings.gso = gso;
    settings.gso_dg_size = 32;
    settings.gso_bf_size = sizeof(buffer);
    stream.test = &test;
    stream.settings = &settings;
    stream.result = &result;
    stream.buffer = buffer;
    stream.packet_count = initial_count;
    stream.socket = -1;
    result.bytes_sent = 1000;
    result.bytes_sent_this_interval = 400;

    /* An invalid descriptor (or unsupported GSO) gives a real local error. */
    for (attempt = 0; attempt < 2; attempt++) {
        memset(buffer, 0, sizeof(buffer));
        r = iperf_udp_send(&stream);
        if (r != NET_HARDERROR || stream.packet_count != initial_count ||
            result.bytes_sent != 1000 || result.bytes_sent_this_interval != 400) {
            fprintf(stderr, "UDP send rollback failed (gso=%d, 64bit=%d, attempt=%d)\n",
                    gso, counters_64bit, attempt);
            ret = 1;
        }
        /* A retry must prepare the same sequence numbers for every datagram. */
        for (i = 0; i < datagrams; i++) {
            if (counters_64bit) {
                memcpy(&sequence, buffer + i * 32 + 8, sizeof(sequence));
                sequence = be64toh(sequence);
            } else {
                memcpy(&sequence32, buffer + i * 32 + 8, sizeof(sequence32));
                sequence = ntohl(sequence32);
            }
            expected = initial_count + i + 1;
            if (sequence != expected) {
                fprintf(stderr, "UDP retry sequence mismatch (gso=%d, 64bit=%d, "
                        "attempt=%d, datagram=%d)\n", gso, counters_64bit, attempt, i);
                ret = 1;
            }
        }
    }
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

    /* Keep all setup and checks active with NDEBUG. */
    ret = test_udp_send_rollback(0, 0);
    ret += test_udp_send_rollback(0, 1);
    ret += test_udp_send_rollback(1, 0);
    ret += test_udp_send_rollback(1, 1);
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
