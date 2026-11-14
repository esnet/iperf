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
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"

#include "version.h"

#include "units.h"

static char calls[16];
static size_t call_count;
static int call_overflow;
static signed char reporter_state;
static int stats_done;

static void
peer_termination_record_call(char kind)
{
    if (call_count + 1 >= sizeof(calls)) {
        call_overflow = 1;
        return;
    }
    calls[call_count++] = kind;
    calls[call_count] = '\0';
}

static void
peer_termination_record_stats(struct iperf_test *test)
{
    stats_done = test->done;
    peer_termination_record_call('S');
}

static void
peer_termination_record_report(struct iperf_test *test)
{
    reporter_state = test->state;
    peer_termination_record_call('R');
}

static int
peer_termination_check_case(char role, signed char previous_state, const char *state_name)
{
    int sockets[2];
    int result;
    int passed;
    struct iperf_test *test;
    signed char message = role == 's' ? CLIENT_TERMINATE : SERVER_TERMINATE;
    signed char final_state = role == 's' ? IPERF_DONE : SERVER_TERMINATE;
    int expected_return = role == 's' ? 0 : -1;
    int expected_error = role == 's' ? IECLIENTTERM : IESERVERTERM;
    int expected_done = previous_state == TEST_RUNNING || previous_state == TEST_END;
    const char *expected_calls = previous_state == TEST_RUNNING ? "SR" : "R";

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        perror("socketpair");
        return 1;
    }
    test = iperf_new_test();
    if (test == NULL) {
        fprintf(stderr, "iperf_new_test failed\n");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }
    if (iperf_defaults(test) < 0) {
        fprintf(stderr, "iperf_defaults failed\n");
        close(sockets[0]);
        close(sockets[1]);
        iperf_free_test(test);
        return 1;
    }
    test->role = role;
    test->state = previous_state;
    test->done = previous_state == TEST_END;
    test->ctrl_sck = sockets[0];
    test->stats_callback = peer_termination_record_stats;
    test->reporter_callback = peer_termination_record_report;
    call_count = 0;
    call_overflow = 0;
    calls[0] = '\0';
    reporter_state = 0;
    stats_done = -1;
    cpu_util(NULL);

    if (write(sockets[1], &message, sizeof(message)) != sizeof(message)) {
        perror("write control message");
        close(sockets[0]);
        close(sockets[1]);
        test->ctrl_sck = -1;
        iperf_free_test(test);
        return 1;
    }
    i_errno = 0;
    result = role == 's' ? iperf_handle_message_server(test)
                        : iperf_handle_message_client(test);
    passed = !call_overflow && strcmp(calls, expected_calls) == 0
             && reporter_state == DISPLAY_RESULTS && result == expected_return
             && test->state == final_state && i_errno == expected_error
             && test->done == expected_done
             && (previous_state != TEST_RUNNING || stats_done == 1);

    printf("%c previous=%s calls=%s expected=%s result=%d state=%d error=%d done=%d stats_done=%d: %s\n",
           role, state_name, calls, expected_calls, result, test->state,
           i_errno, test->done, stats_done, passed ? "PASS" : "FAIL");
    close(sockets[0]);
    close(sockets[1]);
    test->ctrl_sck = -1;
    iperf_free_test(test);
    return !passed;
}

static int
test_peer_termination(void)
{
    static const struct {
        signed char value;
        const char *name;
    } states[] = {
        { CREATE_STREAMS, "CREATE_STREAMS" },
        { TEST_START, "TEST_START" },
        { TEST_RUNNING, "TEST_RUNNING" },
        { TEST_END, "TEST_END" }
    };
    size_t i;
    int failures = 0;
    int result;

    alarm(15);
    for (i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        failures += peer_termination_check_case('s', states[i].value, states[i].name);
        failures += peer_termination_check_case('c', states[i].value, states[i].name);
    }

    /* Reading the previous state must not bypass the existing NULL guard. */
    i_errno = 0;
    result = iperf_handle_message_client(NULL);
    if (result != -1 || i_errno != IEINITTEST) {
        fprintf(stderr, "client NULL input: FAIL (result=%d error=%d)\n",
                result, i_errno);
        ++failures;
    } else {
        printf("client NULL input: PASS\n");
    }
    alarm(0);
    printf("9 checks: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
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

    if (test_peer_termination() != 0)
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
