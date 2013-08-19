/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

/* iperf_server_api.c: Functions to be used by an iperf server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <setjmp.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "iperf_util.h"
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "locale.h"


int
iperf_server_listen(struct iperf_test *test)
{
    retry:
    if((test->listener = netannounce(test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
	if (errno == EAFNOSUPPORT && (test->settings->domain == AF_INET6 || test->settings->domain == AF_UNSPEC)) {
	    /* If we get "Address family not supported by protocol", that
	    ** probably means we were compiled with IPv6 but the running
	    ** kernel does not actually do IPv6.  This is not too unusual,
	    ** v6 support is and perhaps always will be spotty.
	    */
	    warning("this system does not seem to support IPv6 - trying IPv4");
	    test->settings->domain = AF_INET;
	    goto retry;
	} else {
	    i_errno = IELISTEN;
	    return -1;
	}
    }

    if (!test->json_output) {
	printf("-----------------------------------------------------------\n");
	printf("Server listening on %d\n", test->server_port);
    }

    // This needs to be changed to reflect if client has different window size
    // make sure we got what we asked for
    /* XXX: This needs to be moved to the stream listener
    if ((x = get_tcp_windowsize(test->listener, SO_RCVBUF)) < 0) {
        // Needs to set some sort of error number/message
        perror("SO_RCVBUF");
        return -1;
    }
    */

    // XXX: This code needs to be moved to after parameter exhange
    /*
    char ubuf[UNIT_LEN];
    int x;

    if (test->protocol->id == Ptcp) {
        if (test->settings->socket_bufsize > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
	    if (test->json_output) 
		printf("TCP window size: %s\n", ubuf);
        } else {
	    if (test->json_output) 
		printf("Using TCP Autotuning\n");
        }
    }
    */
    if (!test->json_output)
	printf("-----------------------------------------------------------\n");

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = (test->listener > test->max_fd) ? test->listener : test->max_fd;

    return 0;
}

int
iperf_accept(struct iperf_test *test)
{
    int s;
    int rbuf = ACCESS_DENIED;
    char cookie[COOKIE_SIZE];
    socklen_t len;
    struct sockaddr_storage addr;

    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IEACCEPT;
        return -1;
    }

    if (test->ctrl_sck == -1) {
        /* Server free, accept new client */
        if (Nread(s, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return -1;
        }

        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->ctrl_sck = s;

	if (iperf_set_send_state(test, PARAM_EXCHANGE) != 0)
            return -1;
        if (iperf_exchange_parameters(test) < 0) {
            return -1;
        }
        if (test->on_connect) {
            test->on_connect(test);
        }
    } else {
        /* XXX: Do we even need to receive cookie if we're just going to deny anyways? */
        if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return -1;
        }
        if (Nwrite(s, (char*) &rbuf, sizeof(int), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return -1;
        }
        close(s);
    }

    return 0;
}


/**************************************************************************/
int
iperf_handle_message_server(struct iperf_test *test)
{
    int rval;
    struct iperf_stream *sp;

    // XXX: Need to rethink how this behaves to fit API
    if ((rval = Nread(test->ctrl_sck, (char*) &test->state, sizeof(signed char), Ptcp)) <= 0) {
        if (rval == 0) {
	    iperf_err(test, "the client has unexpectedly closed the connection");
            i_errno = IECTRLCLOSE;
            test->state = IPERF_DONE;
            return 0;
        } else {
            i_errno = IERECVMESSAGE;
            return -1;
        }
    }

    switch(test->state) {
        case TEST_START:
            break;
        case TEST_END:
            cpu_util(&test->cpu_util);
            test->stats_callback(test);
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
	    if (iperf_set_send_state(test, EXCHANGE_RESULTS) != 0)
                return -1;
            if (iperf_sum_results(test) < 0)
                return -1;
            if (iperf_exchange_results(test) < 0)
                return -1;
	    if (iperf_set_send_state(test, DISPLAY_RESULTS) != 0)
                return -1;
            if (test->on_test_finish)
                test->on_test_finish(test);
            test->reporter_callback(test);
            break;
        case IPERF_DONE:
            break;
        case CLIENT_TERMINATE:
            i_errno = IECLIENTTERM;

            // XXX: Remove this line below!
	    iperf_err(test, "the client has terminated");
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = IPERF_DONE;
            break;
        default:
            i_errno = IEMESSAGE;
            return -1;
    }

    return 0;
}

/* XXX: This function is not used anymore */
void
iperf_test_reset(struct iperf_test *test)
{
    struct iperf_stream *sp;

    close(test->ctrl_sck);

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }
    if (test->timer != NULL) {
	tmr_cancel(test->timer);
	test->timer = NULL;
    }
    if (test->stats_timer != NULL) {
	tmr_cancel(test->stats_timer);
	test->stats_timer = NULL;
    }
    if (test->reporter_timer != NULL) {
	tmr_cancel(test->reporter_timer);
	test->reporter_timer = NULL;
    }

    SLIST_INIT(&test->streams);

    test->role = 's';
    set_protocol(test, Ptcp);
    test->omit = OMIT;
    test->duration = DURATION;
    test->state = 0;
    test->server_hostname = NULL;

    test->ctrl_sck = -1;
    test->prot_listener = -1;

    test->bytes_sent = 0;

    test->reverse = 0;
    test->sender = 0;
    test->sender_has_retransmits = 0;
    test->no_delay = 0;

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = test->listener;
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = 0;
    test->settings->mss = 0;
    memset(test->cookie, 0, COOKIE_SIZE); 
}

static void
server_omit_timer_proc(TimerClientData client_data, struct timeval *nowP)
{   
    struct iperf_test *test = client_data.p;

    test->omit_timer = NULL;
    test->omitting = 0;
    iperf_reset_stats(test);
    if (test->verbose && !test->json_output)
	printf("Finished omit period, starting real test\n");
}

static int
create_server_omit_timer(struct iperf_test * test)
{
    struct timeval now;
    TimerClientData cd; 

    if (gettimeofday(&now, NULL) < 0) {
	i_errno = IEINITTEST;
	return -1; 
    }
    test->omitting = 1;
    cd.p = test;
    test->omit_timer = tmr_create(&now, server_omit_timer_proc, cd, test->omit * SEC_TO_US, 0); 
    if (test->omit_timer == NULL) {
	i_errno = IEINITTEST;
	return -1;
    }
    return 0;
}

static void
cleanup_server(struct iperf_test *test)
{
    /* Close open test sockets */
    close(test->ctrl_sck);
    close(test->listener);
}

int
iperf_run_server(struct iperf_test *test)
{
    int result, s, streams_accepted;
    fd_set read_set, write_set;
    struct iperf_stream *sp;
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

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        return -1;
    }

    // Begin calculating CPU utilization
    cpu_util(NULL);

    test->state = IPERF_START;
    streams_accepted = 0;

    (void) gettimeofday(&now, NULL);
    while (test->state != IPERF_DONE) {

        memcpy(&read_set, &test->read_set, sizeof(fd_set));
        memcpy(&write_set, &test->write_set, sizeof(fd_set));

        result = select(test->max_fd + 1, &read_set, &write_set, NULL, tmr_timeout(&now));
        if (result < 0 && errno != EINTR) {
	    cleanup_server(test);
            i_errno = IESELECT;
            return -1;
        }
	if (result > 0) {
            if (FD_ISSET(test->listener, &read_set)) {
                if (test->state != CREATE_STREAMS) {
                    if (iperf_accept(test) < 0) {
			cleanup_server(test);
                        return -1;
                    }
                    FD_CLR(test->listener, &read_set);
                }
            }
            if (FD_ISSET(test->ctrl_sck, &read_set)) {
                if (iperf_handle_message_server(test) < 0) {
		    cleanup_server(test);
                    return -1;
		}
                FD_CLR(test->ctrl_sck, &read_set);                
            }

            if (test->state == CREATE_STREAMS) {
                if (FD_ISSET(test->prot_listener, &read_set)) {
    
                    if ((s = test->protocol->accept(test)) < 0) {
			cleanup_server(test);
                        return -1;
		    }

                    if (!is_closed(s)) {
                        sp = iperf_new_stream(test, s);
                        if (!sp) {
			    cleanup_server(test);
                            return -1;
			}

                        FD_SET(s, &test->read_set);
                        FD_SET(s, &test->write_set);
                        test->max_fd = (s > test->max_fd) ? s : test->max_fd;

                        streams_accepted++;
                        if (test->on_new_stream)
                            test->on_new_stream(sp);
                    }
                    FD_CLR(test->prot_listener, &read_set);
                }

                if (streams_accepted == test->num_streams) {
                    if (test->protocol->id != Ptcp) {
                        FD_CLR(test->prot_listener, &test->read_set);
                        close(test->prot_listener);
                    } else { 
                        if (test->no_delay || test->settings->mss || test->settings->socket_bufsize) {
                            FD_CLR(test->listener, &test->read_set);
                            close(test->listener);
                            if ((s = netannounce(test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
				cleanup_server(test);
                                i_errno = IELISTEN;
                                return -1;
                            }
                            test->listener = s;
                            test->max_fd = (s > test->max_fd ? s : test->max_fd);
                            FD_SET(test->listener, &test->read_set);
                        }
                    }
                    test->prot_listener = -1;
		    if (iperf_set_send_state(test, TEST_START) != 0) {
			cleanup_server(test);
                        return -1;
		    }
                    if (iperf_init_test(test) < 0) {
			cleanup_server(test);
                        return -1;
		    }
		    if (create_server_omit_timer(test) < 0) {
			cleanup_server(test);
                        return -1;
		    }
		    if (iperf_set_send_state(test, TEST_RUNNING) != 0) {
			cleanup_server(test);
                        return -1;
		    }
                }
            }

            if (test->state == TEST_RUNNING) {
                if (test->reverse) {
                    // Reverse mode. Server sends.
                    if (iperf_send(test, &write_set) < 0) {
			cleanup_server(test);
                        return -1;
		    }
                } else {
                    // Regular mode. Server receives.
                    if (iperf_recv(test, &read_set) < 0) {
			cleanup_server(test);
                        return -1;
		    }
                }

		/* Run the timers. */
		(void) gettimeofday(&now, NULL);
		tmr_run(&now);
            }
        }
    }

    cleanup_server(test);

    if (test->json_output) {
	if (iperf_json_finish(test) < 0)
	    return -1;
    } 

    return 0;
}
