/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/uio.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_client_api.h"
#include "iperf_error.h"
#include "iperf_util.h"
#include "net.h"
#include "timer.h"


int
iperf_create_streams(struct iperf_test *test)
{
    int i, s;
    struct iperf_stream *sp;

    for (i = 0; i < test->num_streams; ++i) {

        if ((s = test->protocol->connect(test)) < 0)
            return (-1);

        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (test->max_fd < s) ? s : test->max_fd;

        sp = iperf_new_stream(test, s);
        if (!sp)
            return (-1);

        /* Perform the new stream callback */
        if (test->on_new_stream)
            test->on_new_stream(sp);
    }

    return (0);
}

int
iperf_handle_message_client(struct iperf_test *test)
{
    int rval, perr;

    if ((rval = read(test->ctrl_sck, &test->state, sizeof(char))) <= 0) {
        if (rval == 0) {
            i_errno = IECTRLCLOSE;
            return (-1);
        } else {
            i_errno = IERECVMESSAGE;
            return (-1);
        }
    }

    switch (test->state) {
        case PARAM_EXCHANGE:
            if (iperf_exchange_parameters(test) < 0)
                return (-1);
            if (test->on_connect)
                test->on_connect(test);
            break;
        case CREATE_STREAMS:
            if (iperf_create_streams(test) < 0)
                return (-1);
            break;
        case TEST_START:
            if (iperf_init_test(test) < 0)
                return (-1);
            break;
        case TEST_RUNNING:
            break;
        case EXCHANGE_RESULTS:
            if (iperf_exchange_results(test) < 0)
                return (-1);
            break;
        case DISPLAY_RESULTS:
            if (test->on_test_finish)
                test->on_test_finish(test);
            iperf_client_end(test);
            break;
        case IPERF_DONE:
            break;
        case SERVER_TERMINATE:
            i_errno = IESERVERTERM;
            return (-1);
        case ACCESS_DENIED:
            i_errno = IEACCESSDENIED;
            return (-1);
        case SERVER_ERROR:
            if (Nread(test->ctrl_sck, &i_errno, sizeof(i_errno), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return (-1);
            }
            i_errno = ntohl(i_errno);
            if (Nread(test->ctrl_sck, &perr, sizeof(perr), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return (-1);
            }
            errno = ntohl(perr);
            return (-1);
        default:
            i_errno = IEMESSAGE;
            return (-1);
    }

    return (0);
}



/* iperf_connect -- client to server connection function */
int
iperf_connect(struct iperf_test *test)
{
    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);

    make_cookie(test->cookie);

    /* Create and connect the control channel */
    test->ctrl_sck = netdial(test->settings->domain, Ptcp, test->bind_address, test->server_hostname, test->server_port);
    if (test->ctrl_sck < 0) {
        i_errno = IECONNECT;
        return (-1);
    }

    if (Nwrite(test->ctrl_sck, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IESENDCOOKIE;
        return (-1);
    }

    FD_SET(test->ctrl_sck, &test->read_set);
    FD_SET(test->ctrl_sck, &test->write_set);
    test->max_fd = (test->ctrl_sck > test->max_fd) ? test->ctrl_sck : test->max_fd;

    return (0);
}


int
iperf_client_end(struct iperf_test *test)
{
    struct iperf_stream *sp;

    /* Close all stream sockets */
    SLIST_FOREACH(sp, &test->streams, streams) {
        close(sp->socket);
    }

    /* show final summary */
    test->reporter_callback(test);

    test->state = IPERF_DONE;
    if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
        i_errno = IESENDMESSAGE;
        return (-1);
    }

    return (0);
}


int
iperf_run_client(struct iperf_test * test)
{
    int result;
    fd_set temp_read_set, temp_write_set;
    struct timeval tv;
    time_t sec, usec;

    /* Start the client and connect to the server */
    if (iperf_connect(test) < 0) {
        return (-1);
    }

    // Begin calculating CPU utilization
    cpu_util(NULL);

    while (test->state != IPERF_DONE) {

        memcpy(&temp_read_set, &test->read_set, sizeof(fd_set));
        memcpy(&temp_write_set, &test->write_set, sizeof(fd_set));
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        result = select(test->max_fd + 1, &temp_read_set, &temp_write_set, NULL, &tv);
        if (result < 0 && errno != EINTR) {
            i_errno = IESELECT;
            return (-1);
        } else if (result > 0) {
            if (FD_ISSET(test->ctrl_sck, &temp_read_set)) {
                if (iperf_handle_message_client(test) < 0)
                    return (-1);
                FD_CLR(test->ctrl_sck, &temp_read_set);
            }

            if (test->state == TEST_RUNNING) {
                if (test->reverse) {
                    // Reverse mode. Client receives.
                    if (iperf_recv(test) < 0)
                        return (-1);
                } else {
                    // Regular mode. Client sends.
                    if (iperf_send(test) < 0)
                        return (-1);
                }

                /* Perform callbacks */
                if (timer_expired(test->stats_timer)) {
                    test->stats_callback(test);
                    sec = (time_t) test->stats_interval;
                    usec = (test->stats_interval - sec) * SEC_TO_US;
                    if (update_timer(test->stats_timer, sec, usec) < 0)
                        return (-1);
                }
                if (timer_expired(test->reporter_timer)) {
                    test->reporter_callback(test);
                    sec = (time_t) test->reporter_interval;
                    usec = (test->reporter_interval - sec) * SEC_TO_US;
                    if (update_timer(test->reporter_timer, sec, usec) < 0)
                        return (-1);
                }

                /* Send TEST_END if all data has been sent or timer expired */
                if (all_data_sent(test) || timer_expired(test->timer)) {
                    cpu_util(&test->cpu_util);
                    test->stats_callback(test);
                    test->state = TEST_END;
                    if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                        i_errno = IESENDMESSAGE;
                        return (-1);
                    }
                }
            }
        }
    }

    return (0);
}
