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
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"

#include "version.h"

#include "units.h"

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

/* Exercise real cleanup callbacks without starting traffic or worker threads. */
static int cleanup_mutations;
static int cleanup_report_calls;
static int cleanup_json_calls;
static int cleanup_callback_failures;
static int cleanup_error;
static int cleanup_errno;
static char cleanup_error_string[1024];

static void
cleanup_secondary_error(struct iperf_test *test)
{
    /* An application callback can encounter independent API and OS errors. */
    if (set_protocol(test, -1) != -1 || i_errno != IEPROTOCOL)
        ++cleanup_callback_failures;
    if (close(-1) != -1 || errno != EBADF)
        ++cleanup_callback_failures;
}

static void
cleanup_reporter(struct iperf_test *test)
{
    ++cleanup_report_calls;
    cleanup_error = i_errno;
    cleanup_errno = errno;
    snprintf(cleanup_error_string, sizeof(cleanup_error_string), "%s",
             iperf_strerror(cleanup_error));
    if (cleanup_mutations & 1)
        cleanup_secondary_error(test);
}

static void
cleanup_json(struct iperf_test *test, char *output)
{
    cJSON *document, *error;

    ++cleanup_json_calls;
    document = cJSON_Parse(output);
    error = cJSON_GetObjectItem(document, "error");
    if (!cJSON_IsString(error) ||
        strcmp(error->valuestring, cleanup_error_string) != 0)
        ++cleanup_callback_failures;
    cJSON_Delete(document);
    if (cleanup_mutations & 2)
        cleanup_secondary_error(test);
}

static int
check_client_cleanup_errors(void)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct iperf_test *test;
    int blocker, mode, result, returned_error, returned_errno, failed = 0;

    /* Reserve an owned loopback port but do not listen.  No peer is needed;
     * platforms may refuse the connection or reach the bounded timeout. */
    blocker = socket(AF_INET, SOCK_STREAM, 0);
    if (blocker < 0) {
        perror("cleanup test socket");
        return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(blocker, (struct sockaddr *) &addr, sizeof(addr)) < 0 ||
        getsockname(blocker, (struct sockaddr *) &addr, &addrlen) < 0) {
        perror("cleanup test bind/getsockname");
        close(blocker);
        return 1;
    }

    for (mode = 0; mode < 6; ++mode) {
        /* Modes 4 and 5 also exercise the non-JSON return path. */
        cleanup_mutations = mode < 4 ? mode : mode - 4;
        cleanup_report_calls = cleanup_json_calls = cleanup_callback_failures = 0;
        cleanup_error = cleanup_errno = 0;
        cleanup_error_string[0] = '\0';
        test = iperf_new_test();
        if (test == NULL) {
            close(blocker);
            return 1;
        }
        if (iperf_defaults(test) < 0) {
            iperf_free_test(test);
            close(blocker);
            return 1;
        }
        iperf_set_test_role(test, 'c');
        iperf_set_test_server_hostname(test, "127.0.0.1");
        iperf_set_test_server_port(test, ntohs(addr.sin_port));
        iperf_set_test_connect_timeout(test, 100);
        iperf_set_test_json_output(test, mode < 4);
        test->reporter_callback = cleanup_reporter;
        iperf_set_test_json_callback(test, cleanup_json);

        result = iperf_run_client(test);
        returned_error = i_errno;
        returned_errno = errno;
        /* Setup, callback reachability and checks remain active with NDEBUG. */
        if (result != -1 || cleanup_report_calls != 1 || cleanup_json_calls != (mode < 4) ||
            cleanup_callback_failures != 0 || cleanup_error != IECONNECT ||
            (cleanup_errno != ECONNREFUSED && cleanup_errno != ETIMEDOUT) ||
            returned_error != cleanup_error || returned_errno != cleanup_errno) {
            fprintf(stderr, "cleanup errors mode %d: result=%d callbacks=%d/%d "
                    "callback failures=%d original=%d/%d returned=%d/%d\n",
                    mode, result, cleanup_report_calls, cleanup_json_calls,
                    cleanup_callback_failures, cleanup_error, cleanup_errno,
                    returned_error, returned_errno);
            failed = 1;
        }
        iperf_free_test(test);
    }
    close(blocker);
    return failed;
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
    ret += check_client_cleanup_errors();

    iperf_free_test(test);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}
