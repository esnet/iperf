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
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iperf.h"
#include "iperf_api.h"

#include "version.h"

#include "units.h"
#include "net.h"

/* Exercise result exchange and reporting without relying on actual UDP loss. */
static void
test_udp_loss_results(int num_streams, int omit, int legacy_peer, int loss,
                      int json_output)
{
    struct iperf_test *test = iperf_new_test();
    struct iperf_stream *sp;
    cJSON *results, *streams, *stream, *summary, *udp;
    char *payload;
    uint32_t length;
    int control[2], data[2], sockets[4], i;
    int64_t expected_loss = 0, expected_packets = 0;
    int unknown_loss = legacy_peer && omit && loss;
    FILE *output;

    assert(test != NULL);
    assert(iperf_defaults(test) == 0);
    iperf_set_test_role(test, 'c');
    assert(set_protocol(test, Pudp) == 0);
    test->num_streams = num_streams;
    test->omit = omit;
    test->json_output = json_output;
    output = tmpfile();
    assert(output != NULL);
    test->outfile = output;
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, control) == 0);
    test->ctrl_sck = control[0];

    results = cJSON_CreateObject();
    assert(results != NULL);
    cJSON_AddNumberToObject(results, "cpu_util_total", 0);
    cJSON_AddNumberToObject(results, "cpu_util_user", 0);
    cJSON_AddNumberToObject(results, "cpu_util_system", 0);
    cJSON_AddNumberToObject(results, "sender_has_retransmits", -1);
    streams = cJSON_AddArrayToObject(results, "streams");
    assert(streams != NULL);

    for (i = 0; i < num_streams; ++i) {
        int errors = loss * (i + 1);
        int omitted_errors = omit && loss ? i + 1 : 0;
        int omitted_packets = omit ? 100 : 0;

        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, data) == 0);
        sp = iperf_new_stream(test, data[0], 1);
        assert(sp != NULL);
        sockets[i] = data[0];
        close(data[1]);
        sp->packet_count = 1000;
        sp->omitted_packet_count = omitted_packets;
        sp->result->bytes_sent = 100000;
        sp->result->bytes_sent_omit = omitted_packets * 100;
        sp->result->bytes_sent_this_interval = 100000;
        iperf_time_now(&sp->result->start_time);
        --sp->result->start_time.secs;
        sp->result->start_time_fixed = sp->result->start_time;

        stream = cJSON_CreateObject();
        assert(stream != NULL);
        cJSON_AddItemToArray(streams, stream);
        cJSON_AddNumberToObject(stream, "id", sp->id);
        cJSON_AddNumberToObject(stream, "bytes", (1000 - errors) * 100);
        cJSON_AddNumberToObject(stream, "retransmits", -1);
        cJSON_AddNumberToObject(stream, "jitter", 0.001);
        cJSON_AddNumberToObject(stream, "errors", errors);
        cJSON_AddNumberToObject(stream, "packets", 1000);
        if (!legacy_peer) {
            cJSON_AddNumberToObject(stream, "omitted_errors", omitted_errors);
            cJSON_AddNumberToObject(stream, "omitted_packets", omitted_packets);
        }
        expected_loss += errors - omitted_errors;
        expected_packets += 1000 - omitted_packets;
    }

    /* Queue the peer's length-prefixed JSON response on the control channel. */
    payload = cJSON_PrintUnformatted(results);
    assert(payload != NULL);
    length = htonl(strlen(payload));
    assert(Nwrite(control[1], (char *) &length, sizeof(length), Ptcp) == sizeof(length));
    assert(Nwrite(control[1], payload, strlen(payload), Ptcp) == (int) strlen(payload));
    cJSON_free(payload);
    cJSON_Delete(results);
    iperf_stats_callback(test);
    assert(iperf_exchange_results(test) == 0);

    /* With an omit period an old peer cannot identify the omitted losses. */
    if (legacy_peer && omit) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            assert(sp->omitted_cnt_error == (unknown_loss ? -1 : 0));
        }
    } else {
        if (json_output)
            assert(iperf_json_start(test) == 0);
        test->state = DISPLAY_RESULTS;
        iperf_reporter_callback(test);
        if (json_output) {
            summary = cJSON_GetObjectItem(test->json_end, "sum_received");
            assert(summary != NULL);
            assert(cJSON_GetObjectItem(summary, "lost_packets")->valueint == expected_loss);
            assert(cJSON_GetObjectItem(summary, "packets")->valueint == expected_packets);
            assert(fabs(cJSON_GetObjectItem(summary, "lost_percent")->valuedouble -
                        100.0 * expected_loss / expected_packets) < 0.000001);
            streams = cJSON_GetObjectItem(test->json_end, "streams");
            assert(cJSON_GetArraySize(streams) == num_streams);
            for (i = 0; i < num_streams; ++i) {
                udp = cJSON_GetObjectItem(cJSON_GetArrayItem(streams, i), "udp");
                assert(udp != NULL);
                assert(cJSON_GetObjectItem(udp, "lost_packets")->valueint ==
                       (loss - (omit && loss ? 1 : 0)) * (i + 1));
            }
            cJSON_Delete(test->json_top);
            test->json_top = NULL;
        } else {
            char line[1024], *counts;
            int receiver_lines = 0, sum_lines = 0;
            int64_t reported_loss, reported_packets;
            double percent;

            rewind(output);
            while (fgets(line, sizeof(line), output) != NULL) {
                if (strstr(line, "receiver") == NULL)
                    continue;
                counts = strstr(line, " ms ");
                assert(counts != NULL);
                assert(sscanf(counts + 4, "%" SCNd64 "/%" SCNd64 " (%lf%%)",
                              &reported_loss, &reported_packets, &percent) == 3);
                if (strstr(line, "[SUM]") != NULL) {
                    ++sum_lines;
                    if (reported_loss != expected_loss)
                        fprintf(stderr, "UDP SUM: expected %" PRId64 ", got %" PRId64 "\n",
                                expected_loss, reported_loss);
                    assert(reported_loss == expected_loss);
                    assert(reported_packets == expected_packets);
                } else {
                    ++receiver_lines;
                    assert(reported_loss == (loss - (omit && loss ? 1 : 0)) * receiver_lines);
                    assert(reported_packets == (omit ? 900 : 1000));
                }
            }
            assert(receiver_lines == num_streams);
            assert(sum_lines == (num_streams > 1 ? 1 : 0));
        }
    }

    for (i = 0; i < num_streams; ++i)
        close(sockets[i]);
    close(control[0]);
    close(control[1]);
    iperf_free_test(test);
    fclose(output);
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

    if (ret < 0)
    {
        return -1;
    }
    iperf_free_test(test);

    for (int json = 0; json <= 1; ++json) {
        for (int legacy = 0; legacy <= 1; ++legacy) {
            for (int omit = 0; omit <= 1; ++omit) {
                test_udp_loss_results(1, omit, legacy, 10, json);
                test_udp_loss_results(4, omit, legacy, 10, json);
                test_udp_loss_results(4, omit, legacy, 0, json);
            }
        }
    }
    return 0;
}
