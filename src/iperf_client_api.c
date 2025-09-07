/*
 * iperf, Copyright (c) 2014-2023, The Regents of the University of
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
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <signal.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_locale.h"
#include "iperf_time.h"
#include "net.h"
#include "timer.h"

#if defined(HAVE_TCP_CONGESTION)
#if !defined(TCP_CA_NAME_MAX)
#define TCP_CA_NAME_MAX 16
#endif /* TCP_CA_NAME_MAX */
#endif /* HAVE_TCP_CONGESTION */

void *
iperf_client_worker_run(void *s) {
    struct iperf_stream *sp = (struct iperf_stream *) s;
    struct iperf_test *test = sp->test;

    /* Blocking signal to make sure that signal will be handled by main thread */
    sigset_t set;
    sigemptyset(&set);
#ifdef SIGTERM
    sigaddset(&set, SIGTERM);
#endif
#ifdef SIGHUP
    sigaddset(&set, SIGHUP);
#endif
#ifdef SIGINT
    sigaddset(&set, SIGINT);
#endif
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
	    i_errno = IEPTHREADSIGMASK;
	    goto cleanup_and_fail;
    }

    /* Allow this thread to be cancelled even if it's in a syscall */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    while (! (test->done) && ! (sp->done)) {
        if (sp->sender) {
            if (iperf_send_mt(sp) < 0) {
                goto cleanup_and_fail;
            }
        }
        else {
            if (iperf_recv_mt(sp) < 0) {
                goto cleanup_and_fail;
            }
        }
    }
    return NULL;

  cleanup_and_fail:
    return NULL;
}

int
iperf_create_streams(struct iperf_test *test, int sender)
{
    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        return -1;
    }
    int i, s;
#if defined(HAVE_TCP_CONGESTION)
    int saved_errno;
#endif /* HAVE_TCP_CONGESTION */
    struct iperf_stream *sp;

    int orig_bind_port = test->bind_port;
    for (i = 0; i < test->num_streams; ++i) {

        test->bind_port = orig_bind_port;
	if (orig_bind_port) {
	    test->bind_port += i;
            // If Bidir make sure send and receive ports are different
            if (!sender && test->mode == BIDIRECTIONAL)
                test->bind_port += test->num_streams;
        }
        s = test->protocol->connect(test);
        test->bind_port = orig_bind_port;
        if (s < 0)
            return -1;

#if defined(HAVE_TCP_CONGESTION)
	if (test->protocol->id == Ptcp) {
	    if (test->congestion) {
		if (setsockopt(s, IPPROTO_TCP, TCP_CONGESTION, test->congestion, strlen(test->congestion)) < 0) {
		    saved_errno = errno;
		    close(s);
		    errno = saved_errno;
		    i_errno = IESETCONGESTION;
		    return -1;
		}
	    }
	    {
		socklen_t len = TCP_CA_NAME_MAX;
		char ca[TCP_CA_NAME_MAX + 1];
                int rc;
		rc = getsockopt(s, IPPROTO_TCP, TCP_CONGESTION, ca, &len);
                if (rc < 0 && test->congestion) {
		    saved_errno = errno;
		    close(s);
		    errno = saved_errno;
		    i_errno = IESETCONGESTION;
		    return -1;
		}
	        if (test->congestion_used) {
	          if (test->debug)
	            printf("Overriding existing congestion algorithm: %s\n", test->congestion_used);
	          free(test->congestion_used);
	        }
                // Set actual used congestion alg, or set to unknown if could not get it
                if (rc < 0)
                    test->congestion_used = strdup("unknown");
                else
                    test->congestion_used = strdup(ca);
		if (test->debug) {
		    printf("Congestion algorithm is %s\n", test->congestion_used);
		}
	    }
	}
#endif /* HAVE_TCP_CONGESTION */

        sp = iperf_new_stream(test, s, sender);
        if (!sp)
            return -1;

        /* Perform the new stream callback */
        if (test->on_new_stream)
            test->on_new_stream(sp);
    }

    return 0;
}

static void
test_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    test->timer = NULL;
    test->done = 1;
}

static void
client_stats_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->stats_callback)
	test->stats_callback(test);
}

static void
client_reporter_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    if (test->done)
        return;
    if (test->reporter_callback)
	test->reporter_callback(test);
}

static int
create_client_timers(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd;
    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        i_errno = IEINITTEST;
        return -1;
    }

    if (iperf_time_now(&now) < 0) {
	i_errno = IEINITTEST;
	return -1;
    }
    cd.p = test;
    test->timer = test->stats_timer = test->reporter_timer = NULL;
    if (test->duration != 0) {
	test->done = 0;
        test->timer = tmr_create(&now, test_timer_proc, cd, ( test->duration + test->omit ) * SEC_TO_US, 0);
        if (test->timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
	}
    }
    if (test->stats_interval != 0) {
        test->stats_timer = tmr_create(&now, client_stats_timer_proc, cd, test->stats_interval * SEC_TO_US, 1);
        if (test->stats_timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
	}
    }
    if (test->reporter_interval != 0) {
        test->reporter_timer = tmr_create(&now, client_reporter_timer_proc, cd, test->reporter_interval * SEC_TO_US, 1);
        if (test->reporter_timer == NULL) {
            i_errno = IEINITTEST;
            return -1;
	}
    }
    return 0;
}

static void
client_omit_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_test *test = client_data.p;

    test->omit_timer = NULL;
    test->omitting = 0;
    iperf_reset_stats(test);
    if (test->verbose && !test->json_output && test->reporter_interval == 0)
        iperf_printf(test, "%s", report_omit_done);

    /* Reset the timers. */
    if (test->stats_timer != NULL)
        tmr_reset(nowP, test->stats_timer);
    if (test->reporter_timer != NULL)
        tmr_reset(nowP, test->reporter_timer);
}

static int
create_client_omit_timer(struct iperf_test * test)
{
    struct iperf_time now;
    TimerClientData cd;
    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        return -1;
    }

    if (test->omit == 0) {
	test->omit_timer = NULL;
        test->omitting = 0;
    } else {
	if (iperf_time_now(&now) < 0) {
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
    }
    return 0;
}

int
iperf_handle_message_client(struct iperf_test *test)
{
    int rval;
    int32_t err;

    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
	i_errno = IEINITTEST;
        return -1;
    }

    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "Reading new State from the Server - current state is %d-%s\n", test->state, state_to_text(test->state));
    }

    /*!!! Why is this read() and not Nread()? */
    if ((rval = read(test->ctrl_sck, (char*) &test->state, sizeof(signed char))) <= 0) {
        if (rval == 0) {
            i_errno = IECTRLCLOSE;
            return -1;
        } else {
            i_errno = IERECVMESSAGE;
            return -1;
        }
    }

    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "State change: client received and changed State to %d-%s\n", test->state, state_to_text(test->state));
    }

    switch (test->state) {
        case PARAM_EXCHANGE:
            if (iperf_exchange_parameters(test) < 0)
                return -1;
            if (test->on_connect)
                test->on_connect(test);
            break;
        case CREATE_STREAMS:
            if (test->mode == BIDIRECTIONAL)
            {
                if (iperf_create_streams(test, 1) < 0)
                    return -1;
                if (iperf_create_streams(test, 0) < 0)
                    return -1;
            }
            else if (iperf_create_streams(test, test->mode) < 0)
                return -1;
            break;
        case TEST_START:
            if (iperf_init_test(test) < 0)
                return -1;
            if (create_client_timers(test) < 0)
                return -1;
            if (create_client_omit_timer(test) < 0)
                return -1;
	    if (test->mode)
		if (iperf_create_send_timers(test) < 0)
		    return -1;
            break;
        case TEST_RUNNING:
            break;
        case EXCHANGE_RESULTS:
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

	    /*
	     * Temporarily be in DISPLAY_RESULTS phase so we can get
	     * ending summary statistics.
	     */
	    signed char oldstate = test->state;
	    cpu_util(test->cpu_util);
	    test->state = DISPLAY_RESULTS;
	    test->reporter_callback(test);
	    test->state = oldstate;
            return -1;
        case ACCESS_DENIED:
            i_errno = IEACCESSDENIED;
            return -1;
        case SERVER_ERROR:
            if (Nread(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return -1;
            }
	    i_errno = ntohl(err);
            if (Nread(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                i_errno = IECTRLREAD;
                return -1;
            }
            errno = ntohl(err);
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
    int opt;
    socklen_t len;

    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        return -1;
    }
    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);

    make_cookie(test->cookie);

    /* Create and connect the control channel */
    if (test->ctrl_sck < 0)
	// Create the control channel using an ephemeral port
	test->ctrl_sck = netdial(test->settings->domain, Ptcp, test->bind_address, test->bind_dev, 0, test->server_hostname, test->server_port, test->settings->connect_timeout);
    if (test->ctrl_sck < 0) {
        i_errno = IECONNECT;
        return -1;
    }

    // set TCP_NODELAY for lower latency on control messages
    int flag = 1;
    if (setsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int))) {
        i_errno = IESETNODELAY;
        return -1;
    }

#if defined (HAVE_TCP_KEEPALIVE)
        // Set Control Connection TCP Keepalive (especially useful for long UDP test sessions)
        if (iperf_set_control_keepalive(test) < 0)
            return -1;
#endif //HAVE_TCP_KEEPALIVE

#if defined(HAVE_TCP_USER_TIMEOUT)
    if ((opt = test->settings->snd_timeout)) {
        if (setsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_USER_TIMEOUT, &opt, sizeof(opt)) < 0) {
            i_errno = IESETUSERTIMEOUT;
            return -1;
        }
    }
#endif /* HAVE_TCP_USER_TIMEOUT */

    if (Nwrite(test->ctrl_sck, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
        i_errno = IESENDCOOKIE;
        return -1;
    }

    FD_SET(test->ctrl_sck, &test->read_set);
    if (test->ctrl_sck > test->max_fd) test->max_fd = test->ctrl_sck;

    len = sizeof(opt);
    if (getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len) < 0) {
        test->ctrl_sck_mss = 0;
    }
    else {
        if (opt > 0 && opt <= MAX_UDP_BLOCKSIZE) {
            test->ctrl_sck_mss = opt;
        }
        else {
            char str[WARN_STR_LEN];
            snprintf(str, sizeof(str),
                     "Ignoring nonsense TCP MSS %d", opt);
            warning(str);

            test->ctrl_sck_mss = 0;
        }
    }

    if (test->verbose) {
	printf("Control connection MSS %d\n", test->ctrl_sck_mss);
    }

    /*
     * If we're doing a UDP test and the block size wasn't explicitly
     * set, then use the known MSS of the control connection to pick
     * an appropriate default.  If we weren't able to get the
     * MSS for some reason, then default to something that should
     * work on non-jumbo-frame Ethernet networks.  The goal is to
     * pick a reasonable default that is large but should get from
     * sender to receiver without any IP fragmentation.
     *
     * We assume that the control connection is routed the same as the
     * data packets (thus has the same PMTU).  Also in the case of
     * --reverse tests, we assume that the MTU is the same in both
     * directions.  Note that even if the algorithm guesses wrong,
     * the user always has the option to override.
     */
    if (test->protocol->id == Pudp) {
	if (test->settings->blksize == 0) {
	    if (test->ctrl_sck_mss) {
		test->settings->blksize = test->ctrl_sck_mss;
	    }
	    else {
		test->settings->blksize = DEFAULT_UDP_BLKSIZE;
	    }
	    if (test->verbose) {
		printf("Setting UDP block size to %d\n", test->settings->blksize);
	    }
	}

	/*
	 * Regardless of whether explicitly or implicitly set, if the
	 * block size is larger than the MSS, print a warning.
	 */
	if (test->ctrl_sck_mss > 0 &&
	    test->settings->blksize > test->ctrl_sck_mss) {
	    char str[WARN_STR_LEN];
	    snprintf(str, sizeof(str),
		     "UDP block size %d exceeds TCP MSS %d, may result in fragmentation / drops", test->settings->blksize, test->ctrl_sck_mss);
	    warning(str);
	}
    }

    return 0;
}


int
iperf_client_end(struct iperf_test *test)
{
    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        return -1;
    }
    struct iperf_stream *sp;

    /* Close all stream sockets */
    SLIST_FOREACH(sp, &test->streams, streams) {
        close(sp->socket);
    }

    /* show final summary */
    test->reporter_callback(test);

    /* Send response only if no error in server */
    if (test->state > 0) {
        if (iperf_set_send_state(test, IPERF_DONE) != 0)
            return -1;
    }

    /* Close control socket */
    if (test->ctrl_sck >= 0)
        close(test->ctrl_sck);

    return 0;
}


int
iperf_run_client(struct iperf_test * test)
{
    int startup;
    int result = 0;
    fd_set read_set, write_set;
    struct iperf_time now;
    struct timeval* timeout = NULL;
    struct iperf_stream *sp;
    struct iperf_time last_receive_time;
    struct iperf_time diff_time;
    struct timeval used_timeout;
    iperf_size_t last_receive_blocks;
    int64_t t_usecs;
    int64_t timeout_us;
    int64_t rcv_timeout_us;
    int i_errno_save;

    if (NULL == test)
    {
        iperf_err(NULL, "No test\n");
        return -1;
    }

    if (test->logfile)
        if (iperf_open_logfile(test) < 0)
            return -1;

    if (test->affinity != -1)
	if (iperf_setaffinity(test, test->affinity) != 0)
	    return -1;

    if (test->json_output)
	if (iperf_json_start(test) < 0)
	    return -1;

    if (test->json_output) {
	cJSON_AddItemToObject(test->json_start, "version", cJSON_CreateString(version));
	cJSON_AddItemToObject(test->json_start, "system_info", cJSON_CreateString(get_system_info()));
    } else if (test->verbose) {
	iperf_printf(test, "%s\n", version);
	iperf_printf(test, "%s", "");
	iperf_printf(test, "%s\n", get_system_info());
	iflush(test);
    }

    /* Start the client and connect to the server */
    if (iperf_connect(test) < 0)
        goto cleanup_and_fail;

    /* Begin calculating CPU utilization */
    cpu_util(NULL);
    if (test->mode != SENDER)
        rcv_timeout_us = (test->settings->rcv_timeout.secs * SEC_TO_US) + test->settings->rcv_timeout.usecs;
    else
        rcv_timeout_us = 0;

    iperf_time_now(&last_receive_time); // Initialize last time something was received
    last_receive_blocks = 0;

    startup = 1;
    while (test->state != IPERF_DONE) {
	memcpy(&read_set, &test->read_set, sizeof(fd_set));
	memcpy(&write_set, &test->write_set, sizeof(fd_set));
	iperf_time_now(&now);
	timeout = tmr_timeout(&now);

        // In reverse active mode client ensures data is received
        if (test->state == TEST_RUNNING && rcv_timeout_us > 0) {
            timeout_us = -1;
            if (timeout != NULL) {
                used_timeout.tv_sec = timeout->tv_sec;
                used_timeout.tv_usec = timeout->tv_usec;
                timeout_us = (timeout->tv_sec * SEC_TO_US) + timeout->tv_usec;
            }
            /* Cap the maximum select timeout at 1 second */
            if (timeout_us > SEC_TO_US) {
                timeout_us = SEC_TO_US;
            }
            if (timeout_us < 0 || timeout_us > rcv_timeout_us) {
                used_timeout.tv_sec = test->settings->rcv_timeout.secs;
                used_timeout.tv_usec = test->settings->rcv_timeout.usecs;
            }
            timeout = &used_timeout;
        }

#if (defined(__vxworks)) || (defined(__VXWORKS__))
    if (timeout != NULL && timeout->tv_sec == 0 && timeout->tv_usec == 0) {
        taskDelay (1);
	}

    result = select(test->max_fd + 1,
                    &read_set,
                    (test->state == TEST_RUNNING && !test->reverse) ? &write_set : NULL,
                    NULL,
                    timeout);
#else
	result = select(test->max_fd + 1, &read_set, &write_set, NULL, timeout);
#endif // __vxworks or __VXWORKS__
	if (result < 0 && errno != EINTR) {
  	    i_errno = IESELECT;
	    goto cleanup_and_fail;
        } else if (result == 0 && test->state == TEST_RUNNING && rcv_timeout_us > 0) {
            /*
             * If nothing was received in non-reverse running state
             * then probably something got stuck - either client,
             * server or network, and test should be terminated./
             */
            iperf_time_now(&now);
            if (iperf_time_diff(&now, &last_receive_time, &diff_time) == 0) {
                t_usecs = iperf_time_in_usecs(&diff_time);
                if (t_usecs > rcv_timeout_us) {
                    /* Idle timeout if no new blocks received */
                    if (test->blocks_received == last_receive_blocks) {
                        i_errno = IENOMSG;
                        goto cleanup_and_fail;
                    }
                }

            }
        }

        /* See if the test is making progress */
        if (test->blocks_received > last_receive_blocks) {
            last_receive_blocks = test->blocks_received;
            last_receive_time = now;
        }

	if (result > 0) {
	    if (FD_ISSET(test->ctrl_sck, &read_set)) {
 	        if (iperf_handle_message_client(test) < 0) {
		    goto cleanup_and_fail;
		}
		FD_CLR(test->ctrl_sck, &read_set);
	    }
	}

	if (test->state == TEST_RUNNING) {

	    /* Is this our first time really running? */
	    if (startup) {
	        startup = 0;

                /* Create and spin up threads */
                pthread_attr_t attr;
                if (pthread_attr_init(&attr) != 0) {
                    i_errno = IEPTHREADATTRINIT;
                    goto cleanup_and_fail;
                }

                SLIST_FOREACH(sp, &test->streams, streams) {
                    if (pthread_create(&(sp->thr), &attr, &iperf_client_worker_run, sp) != 0) {
                        i_errno = IEPTHREADCREATE;
                        goto cleanup_and_fail;
                    }
                    sp->thread_created = 1;
                    if (test->debug_level >= DEBUG_LEVEL_INFO) {
                        iperf_printf(test, "Thread FD %d created\n", sp->socket);
                    }
                }
                if (test->debug_level >= DEBUG_LEVEL_INFO) {
                    iperf_printf(test, "All threads created\n");
                }
                if (pthread_attr_destroy(&attr) != 0) {
                    i_errno = IEPTHREADATTRDESTROY;
                    goto cleanup_and_fail;
                }

	    }

            /* Run the timers. */
            iperf_time_now(&now);
            tmr_run(&now);

	    /*
	     * Is the test done yet?  We have to be out of omitting
	     * mode, and then we have to have fulfilled one of the
	     * ending criteria, either by times, bytes, or blocks.
	     * The bytes and blocks tests needs to handle both the
	     * cases of the client being the sender and the client
	     * being the receiver.
	     */
	    if ((!test->omitting) &&
	        (test->done ||
	         (test->settings->bytes != 0 && (test->bytes_sent >= test->settings->bytes ||
						 test->bytes_received >= test->settings->bytes)) ||
	         (test->settings->blocks != 0 && (test->blocks_sent >= test->settings->blocks ||
						  test->blocks_received >= test->settings->blocks)))) {

                /* Cancel outstanding sender threads */
                SLIST_FOREACH(sp, &test->streams, streams) {
                    if (sp->sender) {
                        int rc;
                        sp->done = 1;
                        if (sp->thread_created == 1) {
                            rc = pthread_cancel(sp->thr);
                            if (rc != 0 && rc != ESRCH) {
                                i_errno = IEPTHREADCANCEL;
                                errno = rc;
                                iperf_err(test, "sender cancel in pthread_cancel - %s", iperf_strerror(i_errno));
                                goto cleanup_and_fail;
                            }
                            rc = pthread_join(sp->thr, NULL);
                            if (rc != 0 && rc != ESRCH) {
                                i_errno = IEPTHREADJOIN;
                                errno = rc;
                                iperf_err(test, "sender cancel in pthread_join - %s", iperf_strerror(i_errno));
                                goto cleanup_and_fail;
                            }
                            if (test->debug_level >= DEBUG_LEVEL_INFO) {
                                iperf_printf(test, "Thread FD %d stopped\n", sp->socket);
                            }
                            sp->thread_created = 0;
                        }
                    }
                }
                if (test->debug_level >= DEBUG_LEVEL_INFO) {
                    iperf_printf(test, "Sender threads stopped\n");
                }

		/* Yes, done!  Send TEST_END. */
		test->done = 1;
		cpu_util(test->cpu_util);
		test->stats_callback(test);
		if (iperf_set_send_state(test, TEST_END) != 0)
                    goto cleanup_and_fail;
	    }
	}
    }

    /* Cancel outstanding receiver threads */
    SLIST_FOREACH(sp, &test->streams, streams) {
        if (!sp->sender) {
            int rc;
            sp->done = 1;
            if (sp->thread_created == 1) {
                rc = pthread_cancel(sp->thr);
                if (rc != 0 && rc != ESRCH) {
                    i_errno = IEPTHREADCANCEL;
                    errno = rc;
                    iperf_err(test, "receiver cancel in pthread_cancel - %s", iperf_strerror(i_errno));
                    goto cleanup_and_fail;
                }
                rc = pthread_join(sp->thr, NULL);
                if (rc != 0 && rc != ESRCH) {
                    i_errno = IEPTHREADJOIN;
                    errno = rc;
                    iperf_err(test, "receiver cancel in pthread_join - %s", iperf_strerror(i_errno));
                    goto cleanup_and_fail;
                }
                if (test->debug_level >= DEBUG_LEVEL_INFO) {
                    iperf_printf(test, "Thread FD %d stopped\n", sp->socket);
                }
                sp->thread_created = 0;
            }
        }
    }
    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "Receiver threads stopped\n");
    }

    if (test->json_output) {
	if (iperf_json_finish(test) < 0)
	    return -1;
    } else {
	iperf_printf(test, "\n");
	iperf_printf(test, "%s", report_done);
    }

    iflush(test);

    return 0;

  cleanup_and_fail:
    /* Cancel all outstanding threads */
    i_errno_save = i_errno;
    SLIST_FOREACH(sp, &test->streams, streams) {
        if (sp->done) {
            continue;
        }
        sp->done = 1;
        int rc;
        if (sp->thread_created == 1) {
            rc = pthread_cancel(sp->thr);
            if (rc != 0 && rc != ESRCH) {
                i_errno = IEPTHREADCANCEL;
                errno = rc;
                iperf_err(test, "cleanup_and_fail in pthread_cancel - %s", iperf_strerror(i_errno));
            }
            rc = pthread_join(sp->thr, NULL); 
            if (rc != 0 && rc != ESRCH) {
                i_errno = IEPTHREADJOIN;
                errno = rc;
                iperf_err(test, "cleanup_and_fail in pthread_join - %s", iperf_strerror(i_errno));
            }
            if (test->debug_level >= DEBUG_LEVEL_INFO) {
                iperf_printf(test, "Thread FD %d stopped\n", sp->socket);
            }
            sp->thread_created = 0;
        }
    }
    if (test->debug_level >= DEBUG_LEVEL_INFO) {
        iperf_printf(test, "All threads stopped\n");
    }
    i_errno = i_errno_save;

    iperf_client_end(test);
    if (test->json_output) {
        cJSON_AddStringToObject(test->json_top, "error", iperf_strerror(i_errno));
        iperf_json_finish(test);
    }
    iflush(test);
    return -1;
}
