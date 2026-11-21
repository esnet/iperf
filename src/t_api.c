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
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"

#include "version.h"

#include "units.h"

static int
check_target_bitrate_number(cJSON *object, const char *key, uint64_t expected, const char *label)
{
    cJSON *item;
    int count = 0, bad_value = 0;

    cJSON_ArrayForEach(item, object) {
        if (item->string != NULL && strcmp(item->string, key) == 0) {
            ++count;
            if (!cJSON_IsNumber(item) || item->valuedouble != (double) expected)
                bad_value = 1;
        }
    }
    if (count != 1 || bad_value) {
        fprintf(stderr, "%s: %s count=%d value_ok=%d expected=%" PRIu64 "\n",
                label, key, count, !bad_value, expected);
        return 1;
    }
    return 0;
}

/* Keep setup and checks active when assertions are disabled. */
#define CHECK_TARGET_BITRATE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "target bitrate: %s failed\n", #condition); \
        failures++; \
        goto cleanup; \
    } \
} while (0)

static int
check_target_bitrate_case(uint64_t rate, uint64_t prior_server_rate, int reject)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct iperf_test *client = iperf_new_test();
    struct iperf_test *server = iperf_new_test();
    int listener = -1, client_fd = -1, server_fd = -1, failures = 0;
    char label[100];
    signed char state;

    snprintf(label, sizeof(label), "server rate=%" PRIu64 " prior=%" PRIu64 " reject=%d",
             rate, prior_server_rate, reject);
    CHECK_TARGET_BITRATE(client != NULL && server != NULL);
    CHECK_TARGET_BITRATE(iperf_defaults(client) == 0 && iperf_defaults(server) == 0);
    iperf_set_test_role(client, 'c');
    iperf_set_test_role(server, 's');
    iperf_set_test_json_output(client, 1);
    iperf_set_test_json_output(server, 1);
    iperf_set_test_server_hostname(client, "127.0.0.1");
    iperf_set_test_rate(client, rate);
    iperf_set_test_rate(server, prior_server_rate);
    CHECK_TARGET_BITRATE(iperf_json_start(client) == 0 && iperf_json_start(server) == 0);
    if (reject)
        server->settings->bitrate_limit = rate / 2;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_TARGET_BITRATE(listener >= 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    CHECK_TARGET_BITRATE(bind(listener, (struct sockaddr *) &addr, sizeof(addr)) == 0);
    CHECK_TARGET_BITRATE(listen(listener, 1) == 0);
    CHECK_TARGET_BITRATE(getsockname(listener, (struct sockaddr *) &addr, &addrlen) == 0);
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_TARGET_BITRATE(client_fd >= 0);
    CHECK_TARGET_BITRATE(connect(client_fd, (struct sockaddr *) &addr, sizeof(addr)) == 0);
    server_fd = accept(listener, NULL, NULL);
    CHECK_TARGET_BITRATE(server_fd >= 0);
    client->ctrl_sck = client_fd;
    server->ctrl_sck = server_fd;
    server->listener = listener;
    client->server_port = server->server_port = ntohs(addr.sin_port);

    CHECK_TARGET_BITRATE(iperf_exchange_parameters(client) == 0);
    if (reject) {
        CHECK_TARGET_BITRATE(iperf_exchange_parameters(server) == -1);
        CHECK_TARGET_BITRATE(i_errno == IETOTALRATE);
        /* The normal server path does not call on_connect on rejection. */
        failures += check_target_bitrate_number(server->json_start, "target_bitrate", rate, label);
    } else {
        CHECK_TARGET_BITRATE(iperf_exchange_parameters(server) == 0);
        CHECK_TARGET_BITRATE(read(client_fd, &state, 1) == 1 && state == CREATE_STREAMS);
        CHECK_TARGET_BITRATE(server->settings->rate == rate);
        iperf_on_connect(client);
        iperf_on_connect(server);
        failures += check_target_bitrate_number(client->json_start, "target_bitrate", rate, "client");
        failures += check_target_bitrate_number(server->json_start, "target_bitrate", rate, label);
        failures += check_target_bitrate_number(client->json_start, "fq_rate", 0, "client");
        failures += check_target_bitrate_number(server->json_start, "fq_rate", 0, label);
        iperf_on_test_start(client);
        iperf_on_test_start(server);
        failures += check_target_bitrate_number(cJSON_GetObjectItemCaseSensitive(client->json_start, "test_start"),
                                 "target_bitrate", rate, "client/test_start");
        failures += check_target_bitrate_number(cJSON_GetObjectItemCaseSensitive(server->json_start, "test_start"),
                                 "target_bitrate", rate, "server/test_start");
    }
cleanup:
    printf("%s: %s\n", label, failures ? "FAIL" : "PASS");
    if (client_fd >= 0)
        close(client_fd);
    if (server_fd >= 0)
        close(server_fd);
    if (listener >= 0)
        close(listener);
    /* iperf_free_test does not own the control sockets or JSON tree. */
    if (client != NULL) {
        client->ctrl_sck = -1;
        cJSON_Delete(client->json_top);
        client->json_top = client->json_start = client->json_connected = NULL;
        client->json_intervals = client->json_end = NULL;
        iperf_free_test(client);
    }
    if (server != NULL) {
        server->ctrl_sck = server->listener = -1;
        cJSON_Delete(server->json_top);
        server->json_top = server->json_start = server->json_connected = NULL;
        server->json_intervals = server->json_end = NULL;
        iperf_free_test(server);
    }
    return failures;
}

#undef CHECK_TARGET_BITRATE

static int
test_target_bitrate(void)
{
    int failures = 0;
    alarm(15);
    failures += check_target_bitrate_case(0, 0, 0);
    failures += check_target_bitrate_case(1234567, 7654321, 0);
    failures += check_target_bitrate_case(UINT64_C(5000000000), 0, 0);
    failures += check_target_bitrate_case(1000000, 0, 1);
    alarm(0);
    return failures != 0;
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

    if (test_target_bitrate() != 0)
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
