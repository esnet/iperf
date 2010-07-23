
/*
 * Copyright (c) 2009, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

/* iperf_server_api.c: Functions to be used by an iperf server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/queue.h>
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
#include "iperf_server_api.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "iperf_error.h"
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
    char ubuf[UNIT_LEN];
    int x;

    if((test->listener = netannounce(Ptcp, NULL, test->server_port)) < 0) {
        i_errno = IELISTEN;
        return (-1);
    }

    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", test->server_port);

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
    if (test->protocol->id == Ptcp) {
        if (test->settings->socket_bufsize > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
            printf("TCP window size: %s\n", ubuf);
        } else {
            printf("Using TCP Autotuning\n");
        }
    }
    */
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
    struct sockaddr_in addr;

    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IEACCEPT;
        return (-1);
    }

    if (test->ctrl_sck == -1) {
        /* Server free, accept new client */
        if (Nread(s, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return (-1);
        }

        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->ctrl_sck = s;

        test->state = PARAM_EXCHANGE;
        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }
        if (iperf_exchange_parameters(test) < 0) {
            return (-1);
        }
        if (test->on_connect) {
            test->on_connect(test);
        }
    } else {
        // XXX: Do we even need to receive cookie if we're just going to deny anyways?
        if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return (-1);
        }
        if (Nwrite(s, &rbuf, sizeof(int), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }
        close(s);
    }

    return (0);
}


/**************************************************************************/
int
iperf_handle_message_server(struct iperf_test *test)
{
    int rval;
    struct iperf_stream *sp;

    // XXX: Need to rethink how this behaves to fit API
    if ((rval = Nread(test->ctrl_sck, &test->state, sizeof(char), Ptcp)) <= 0) {
        if (rval == 0) {
            fprintf(stderr, "The client has unexpectedly closed the connection.\n");
            i_errno = IECTRLCLOSE;
            test->state = IPERF_DONE;
            return (0);
        } else {
            i_errno = IERECVMESSAGE;
            return (-1);
        }
    }

    switch(test->state) {
        case TEST_START:
            break;
        case TEST_END:
            test->stats_callback(test);
            for (sp = test->streams; sp; sp = sp->next) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = EXCHANGE_RESULTS;
            if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            if (iperf_exchange_results(test) < 0)
                return (-1);
            test->state = DISPLAY_RESULTS;
            if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            if (test->on_test_finish)
                test->on_test_finish(test);
            test->reporter_callback(test);
            break;
        case IPERF_DONE:
            break;
        case CLIENT_TERMINATE:
            fprintf(stderr, "The client has terminated.\n");
            for (sp = test->streams; sp; sp = sp->next) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = IPERF_DONE;
            break;
        default:
            i_errno = IEMESSAGE;
            return (-1);
    }

    return (0);
}

void
iperf_test_reset(struct iperf_test *test)
{
    struct iperf_stream *sp, *np;

    close(test->ctrl_sck);

    /* Free streams */
    for (sp = test->streams; sp; sp = np) {
        np = sp->next;
        iperf_free_stream(sp);
    }
    free_timer(test->timer);
    free_timer(test->stats_timer);
    free_timer(test->reporter_timer);
    test->timer = NULL;
    test->stats_timer = NULL;
    test->reporter_timer = NULL;

    test->streams = NULL;

    test->role = 's';
    set_protocol(test, Ptcp);
    test->duration = DURATION;
    test->state = 0;
    test->server_hostname = NULL;

    test->ctrl_sck = -1;
    test->prot_listener = -1;

    test->bytes_sent = 0;

    test->reverse = 0;
    test->no_delay = 0;

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = test->listener;
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = RATE;   /* UDP only */
    test->settings->mss = 0;
    memset(test->cookie, 0, COOKIE_SIZE); 
}

int
iperf_run_server(struct iperf_test *test)
{
    int result, s, streams_accepted;
    fd_set temp_read_set, temp_write_set;
    struct iperf_stream *sp;
    struct timeval tv;

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        return (-1);
    }

    signal(SIGINT, sig_handler);
    if (setjmp(env)) {
        test->state = SERVER_TERMINATE;
        if (test->ctrl_sck >= 0) {
            if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
        }
        return (0);
    }

    for ( ; ; ) {

        test->state = IPERF_START;
        streams_accepted = 0;

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
                if (FD_ISSET(test->listener, &temp_read_set)) {
                    if (test->state != CREATE_STREAMS) {
                        if (iperf_accept(test) < 0) {
                            return (-1);
                        }
                        FD_CLR(test->listener, &temp_read_set);
                    }
                }
                if (FD_ISSET(test->ctrl_sck, &temp_read_set)) {
                    if (iperf_handle_message_server(test) < 0)
                        return (-1);
                    FD_CLR(test->ctrl_sck, &temp_read_set);                
                }

                if (test->state == CREATE_STREAMS) {
                    if (FD_ISSET(test->prot_listener, &temp_read_set)) {
        
                        if ((s = test->protocol->accept(test)) < 0)
                            return (-1);

                        if (!is_closed(s)) {
                            sp = iperf_new_stream(test, s);
                            if (!sp)
                                return (-1);

                            FD_SET(s, &test->read_set);
                            FD_SET(s, &test->write_set);
                            test->max_fd = (s > test->max_fd) ? s : test->max_fd;

                            streams_accepted++;
//                            connect_msg(sp);
                            if (test->on_new_stream)
                                test->on_new_stream(sp);
                        }
                        FD_CLR(test->prot_listener, &temp_read_set);
                    }

                    if (streams_accepted == test->num_streams) {
                        if (test->protocol->id != Ptcp) {
                            FD_CLR(test->prot_listener, &test->read_set);
                            close(test->prot_listener);
                        } else { 
                            if (test->no_delay || test->settings->mss) {
                                FD_CLR(test->listener, &test->read_set);
                                close(test->listener);
                                if ((s = netannounce(Ptcp, NULL, test->server_port)) < 0) {
                                    i_errno = IELISTEN;
                                    return (-1);
                                }
                                test->listener = s;
                                test->max_fd = (s > test->max_fd ? s : test->max_fd);
                                FD_SET(test->listener, &test->read_set);
                            }
                        }
                        test->prot_listener = -1;
                        test->state = TEST_START;
                        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                            i_errno = IESENDMESSAGE;
                            return (-1);
                        }
                        if (iperf_init_test(test) < 0)
                            return (-1);
                        test->state = TEST_RUNNING;
                        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                            i_errno = IESENDMESSAGE;
                            return (-1);
                        }
                    }
                }

                if (test->state == TEST_RUNNING) {
                    if (test->reverse) {
                        // Reverse mode. Server sends.
                        if (iperf_send(test) < 0)
                            return (-1);
                    } else {
                        // Regular mode. Server receives.
                        if (iperf_recv(test) < 0)
                            return (-1);
                    }

                    /* Perform callbacks */
                    if (timer_expired(test->stats_timer)) {
                        test->stats_callback(test);
                        if (update_timer(test->stats_timer, test->stats_interval, 0) < 0)
                            return (-1);
                    }
                    if (timer_expired(test->reporter_timer)) {
                        test->reporter_callback(test);
                        if (update_timer(test->reporter_timer, test->reporter_interval, 0) < 0)
                            return (-1);
                    }
                }
            }
        }

        /* Clean up the last test */
        iperf_test_reset(test);
        printf("\n");

    }

    return (0);
}

