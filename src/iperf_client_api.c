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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <arpa/inet.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "locale.h"
#include "net.h"
#include "timer.h"


int
iperf_create_streams(struct iperf_test *test)
{
    int i, s;
    struct iperf_stream *sp;

    for (i = 0; i < test->num_streams; ++i) {

        if ((s = test->protocol->connect(test)) < 0)
            return -1;

        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (test->max_fd < s) ? s : test->max_fd;

        sp = iperf_new_stream(test, s);
        if (!sp)
            return -1;

        /* Perform the new stream callback */
        if (test->on_new_stream)
            test->on_new_stream(sp);
    }

    return 0;
}

static void
test_timer_proc(TimerClientData client_data, struct timeval *nowP)
{
    struct iperf_test *test = client_data.p;

    test->timer = NULL;
    test->done = 1;
}

static void
stats_timer_proc(TimerClientData client_data, struct timeval *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    test->stats_callback(test);
}

static void
reporter_timer_proc(TimerClientData client_data, struct timeval *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    test->reporter_callback(test);
}

static void
client_omit_timer_proc(TimerClientData client_data, struct timeval *nowP)
{
    struct iperf_test *test = client_data.p;
    TimerClientData cd;

    test->omit_timer = NULL;
    test->omitting = 0;
    iperf_reset_stats(test);
    if (test->verbose && !test->json_output)
        printf("Finished omit period, starting real test\n");

    /* Create timers. */
    if (test->settings->bytes == 0) {
	test->done = 0;
	cd.p = test;
        test->timer = tmr_create(nowP, test_timer_proc, cd, test->duration * SEC_TO_US, 0);
        if (test->timer == NULL) {
            i_errno = IEINITTEST;
            return;
	}
    } 
    if (test->stats_interval != 0) {
	cd.p = test;
        test->stats_timer = tmr_create(nowP, stats_timer_proc, cd, test->stats_interval * SEC_TO_US, 1);
        if (test->stats_timer == NULL) {
            i_errno = IEINITTEST;
            return;
	}
    }
    if (test->reporter_interval != 0) {
	cd.p = test;
        test->reporter_timer = tmr_create(nowP, reporter_timer_proc, cd, test->reporter_interval * SEC_TO_US, 1);
        if (test->reporter_timer == NULL) {
            i_errno = IEINITTEST;
            return;
	}
    }
}

static int
create_client_omit_timer(struct iperf_test * test)
{
    struct timeval now;
    TimerClientData cd;

    if (gettimeofday(&now, NULL) < 0) {
	i_errno = IEINITTEST;
	return -1;
    }
    test->omitting = 1;
    cd.p = test;
    test->omit_timer = tmr_create(&now, client_omit_timer_proc, cd, test->omit * SEC_TO_US, 0);
    if (test->omit_timer == NULL) {
	i_errno = IEINITTEST;
	return -1;
    }
    return 0;
}

int
iperf_handle_message_client(struct iperf_test *test)
{
    int rval, perr;

    if ((rval = read(test->ctrl_sck, (char*) &test->state, sizeof(signed char))) <= 0) {
        if (rval == 0) {
            i_errno = IECTRLCLOSE;
            return -1;
        } else {
            i_errno = IERECVMESSAGE;
            return -1;
        }
    }

    switch (test->state) {
        case PARAM_EXCHANGE:
            if (iperf_exchange_parameters(test) < 0)
                return -1;
            if (test->on_connect)
                test->on_connect(test);
            break;
        case CREATE_STREAMS:
            if (iperf_create_streams(test) < 0)
                return -1;
            break;
        case TEST_START:
            if (iperf_init_test(test) < 0)
                return -1;
            if (create_client_omit_timer(test) < 0)
                return -1;
	    if (!test->reverse)
		if (iperf_create_send_timers(test) < 0)
		    return -1;
            break;
        case TEST_RUNNING:
            break;
        case EXCHANGE_RESULTS:
            if (iperf_sum_results(test) < 0)
                return -1;
            if (iperf_exchange_results(test) < 0)
                return -1;
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
            return -1;
        case ACCESS_DENIED:
            i_errno = IEACCESSDENIED;
            return -1;
        case SERVER_ERROR:
            if (Nread(test->ctrl_sck, (char*) &i_errno, sizeof(i_errno), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return -1;
            }
            i_errno = ntohl(i_errno);
            if (Nread(test->ctrl_sck, (char*) &perr, sizeof(perr), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return -1;
            }
            errno = ntohl(perr);
            return -1;
        default:
            i_errno = IEMESSAGE;
            return -1;
    }

    return 0;
}



/* iperf_connect -- client to server connection function */
int
iperf_connect(struct iperf_test *test)
{
    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);

    make_cookie(test->cookie);

    /* Create and connect the control channel */
    if (test->ctrl_sck < 0)
	test->ctrl_sck = netdial(test->settings->domain, Ptcp, test->bind_address, test->server_hostname, test->server_port);
    if (test->ctrl_sck < 0) {
        i_errno = IECONNECT;
        return -1;
    }

    if (Nwrite(test->ctrl_sck, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IESENDCOOKIE;
        return -1;
    }

    FD_SET(test->ctrl_sck, &test->read_set);
    FD_SET(test->ctrl_sck, &test->write_set);
    test->max_fd = (test->ctrl_sck > test->max_fd) ? test->ctrl_sck : test->max_fd;

    return 0;
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

    if (iperf_set_send_state(test, IPERF_DONE) != 0)
        return -1;

    return 0;
}


static int sigalrm_triggered;

static void
sigalrm_handler(int sig)
{
    sigalrm_triggered = 1;
}


int
iperf_run_client(struct iperf_test * test)
{
    int concurrency_model;
    int startup;
#define CM_SELECT 1
#define CM_SIGALRM 2
    int result;
    fd_set read_set, write_set;
    struct timeval now;

    if (test->json_output)
	if (iperf_json_start(test) < 0)
	    return -1;

    if (test->json_output) {
	cJSON_AddItemToObject(test->json_start, "version", cJSON_CreateString(version));
	cJSON_AddItemToObject(test->json_start, "system_info", cJSON_CreateString(get_system_info()));
    } else if (test->verbose) {
	printf("%s\n", version);
	system("uname -a");
    }

    /* Start the client and connect to the server */
    if (iperf_connect(test) < 0) {
        return -1;
    }

    /* Begin calculating CPU utilization */
    cpu_util(NULL);

    startup = 1;
    concurrency_model = CM_SELECT;	/* always start in select mode */
    (void) gettimeofday(&now, NULL);
    while (test->state != IPERF_DONE) {

	if (concurrency_model == CM_SELECT) {
	    memcpy(&read_set, &test->read_set, sizeof(fd_set));
	    memcpy(&write_set, &test->write_set, sizeof(fd_set));
	    result = select(test->max_fd + 1, &read_set, &write_set, NULL, tmr_timeout(&now));
	    if (result < 0 && errno != EINTR) {
		i_errno = IESELECT;
		return -1;
	    }
	    if (result > 0) {
		if (FD_ISSET(test->ctrl_sck, &read_set)) {
		    if (iperf_handle_message_client(test) < 0) {
			return -1;
		    }
		    FD_CLR(test->ctrl_sck, &read_set);
		}
	    }
	}

	if (test->state == TEST_RUNNING) {

	    /* Is this our first time really running? */
	    if (startup && ! test->omitting) {
	        startup = 0;
		/* Can we switch to SIGALRM mode?  There are a bunch of
		** cases where either it won't work or it's ill-advised.
		*/
		if (test->may_use_sigalrm && test->settings->rate == 0 &&
		    (test->stats_interval == 0 || test->stats_interval > 1) &&
		    (test->reporter_interval == 0 || test->reporter_interval > 1) &&
		    ! test->reverse) {
		    concurrency_model = CM_SIGALRM;
		    test->multisend = 1;
		    signal(SIGALRM, sigalrm_handler);
		    sigalrm_triggered = 0;
		    alarm(1);
		}
	    }

	    if (test->reverse) {
		// Reverse mode. Client receives.
		if (iperf_recv(test, &read_set) < 0) {
		    return -1;
		}
	    } else {
		// Regular mode. Client sends.
		if (iperf_send(test, concurrency_model == CM_SIGALRM ? NULL : &write_set) < 0) {
		    return -1;
		}
	    }

	    if (concurrency_model == CM_SELECT ||
	        (concurrency_model == CM_SIGALRM && sigalrm_triggered)) {
		/* Run the timers. */
		(void) gettimeofday(&now, NULL);
		tmr_run(&now);
	        if (concurrency_model == CM_SIGALRM) {
		    sigalrm_triggered = 0;
		    alarm(1);
		}
	    }

	    /* Is the test done yet? */
	    if (test->omitting)
	        continue;	/* not done */
	    if (test->settings->bytes == 0) {
		if (!test->done)
		    continue;	/* not done */
	    } else {
		if (test->bytes_sent < test->settings->bytes)
		    continue;	/* not done */
	    }
	    /* Yes, done!  Send TEST_END. */
	    cpu_util(&test->cpu_util);
	    test->stats_callback(test);
	    if (iperf_set_send_state(test, TEST_END) != 0)
		return -1;
	    /* And if we were doing SIGALRM, go back to select for the end. */
	    concurrency_model = CM_SELECT;
	}
    }

    if (test->json_output) {
	if (iperf_json_finish(test) < 0)
	    return -1;
    } else
	printf("\niperf Done.\n");

    return 0;
}
