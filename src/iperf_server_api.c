
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
#include "timer.h"
#include "net.h"
#include "units.h"
#include "tcp_window_size.h"
#include "uuid.h"
#include "locale.h"

jmp_buf env;

int
iperf_server_listen(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    int x;

    if((test->listener = netannounce(Ptcp, NULL, test->server_port)) < 0) {
        // Needs to set some sort of error number/message
        perror("netannounce test->listener");
        return -1;
    }

    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", test->server_port);

    // This needs to be changed to reflect if client has different window size
    // make sure we got what we asked for
    /* XXX: This needs to be moved to the stream listener
    if ((x = get_tcp_windowsize(test->listener_sock_tcp, SO_RCVBUF)) < 0) {
        // Needs to set some sort of error number/message
        perror("SO_RCVBUF");
        return -1;
    }
    */

    // XXX: This code needs to be moved to after parameter exhange
    if (test->protocol == Ptcp) {
        if (test->default_settings->socket_bufsize > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
            printf("TCP window size: %s\n", ubuf);
        } else {
            printf("Using TCP Autotuning\n");
        }
    }
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
    char ipl[512], ipr[512];
    socklen_t len;
    struct sockaddr_in addr;
    struct sockaddr_in temp1, temp2;

    if (test->ctrl_sck == -1) {
        if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
            perror("accept");
            return -1;
        }
        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->ctrl_sck = s;

        len = sizeof(struct sockaddr_in);
        if (getsockname(s, (struct sockaddr *) &temp1, &len) < 0)
            perror("getsockname");
        if (getpeername(s, (struct sockaddr *) &temp2, &len) < 0)
            perror("getpeername");

        inet_ntop(AF_INET, (void *) &temp1.sin_addr, ipl, sizeof(ipl));
        inet_ntop(AF_INET, (void *) &temp2.sin_addr, ipr, sizeof(ipr));
        printf(report_peer, s, ipl, ntohs(temp1.sin_port), ipr, ntohs(temp2.sin_port));

        return 0;

    } else {
        // This message needs to be sent to the client
        if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
            perror("accept");
            return -1;
        }
        if (write(s, &rbuf, sizeof(int)) < 0) {
            perror("write");
            return -1;
        }
        close(s);
        return 0;
    }
}

/**************************************************************************/
int
iperf_handle_message_server(struct iperf_test *test)
{
    struct iperf_stream *sp;

    if (read(test->ctrl_sck, &test->state, sizeof(char)) < 0) {
        // XXX: Needs to indicate read error
        return -1;
    }

    switch(test->state) {
        case PARAM_EXCHANGE:
            iperf_exchange_parameters(test);
            break;
        case TEST_START:
            break;
        case TEST_RUNNING:
            break;
        case TEST_END:
            for (sp = test->streams; sp; sp = sp->next) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = EXCHANGE_RESULTS;
            if (write(test->ctrl_sck, &test->state, sizeof(char)) < 0) {
                perror("write EXCHANGE_RESULTS");
                exit(1);
            }
            iperf_exchange_results(test);
            test->state = DISPLAY_RESULTS;
            if (write(test->ctrl_sck, &test->state, sizeof(char)) < 0) {
                perror("write DISPLAY_RESULTS");
                exit(1);
            }
            test->stats_callback(test);
            test->reporter_callback(test);
            break;
        case IPERF_DONE:
            break;
        case CLIENT_TERMINATE:
            fprintf(stderr, "The client has terminated. Exiting...\n");
            exit(1);
        default:
            // XXX: This needs to be replaced by actual error handling
            fprintf(stderr, "How did you get here? test->state = %d\n", test->state);
            return -1;
    }

    return 0;
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

    test->streams = NULL;

    test->role = 's';
    test->protocol = Ptcp;
    test->duration = DURATION;
    test->state = 0;
    test->server_hostname = NULL;

    test->ctrl_sck = -1;
    test->prot_listener = 0;

    test->reverse = 0;
    // test->no_delay = 0 (Do we need this reset between tests?)

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = test->listener;
    
    test->num_streams = 1;
    test->default_settings->socket_bufsize = 0;
    test->default_settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->default_settings->rate = RATE;   /* UDP only */
    memset(test->default_settings->cookie, 0, COOKIE_SIZE); 
}

int
iperf_run_server(struct iperf_test *test)
{
    int result;
    int streams_accepted;
    fd_set temp_read_set, temp_write_set;
    struct timeval tv;

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        // This needs to be replaced by more formal error handling
        fprintf(stderr, "An error occurred. Exiting.\n");
        exit(1);
    }

    signal(SIGINT, sig_handler);
    if (setjmp(env)) {
        fprintf(stderr, "Interrupt received. Exiting...\n");
        test->state = SERVER_TERMINATE;
        if (test->ctrl_sck >= 0) {
            if (write(test->ctrl_sck, &test->state, sizeof(char)) < 0) {
                fprintf(stderr, "Unable to send SERVER_TERMINATE message to client\n");
            }
        }
        exit(1);
    }

    for (;;) {

    test->state = IPERF_START;
    streams_accepted = 0;
    
    while (test->state != IPERF_DONE) {

        memcpy(&temp_read_set, &test->read_set, sizeof(fd_set));
        memcpy(&temp_write_set, &test->write_set, sizeof(fd_set));
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        result = select(test->max_fd + 1, &temp_read_set, &temp_write_set, NULL, &tv);
        if (result < 0 && errno != EINTR) {
            // Change the way this handles errors
            perror("select");
            exit(1);
        } else if (result > 0) {
            if (FD_ISSET(test->listener, &temp_read_set)) {
                if (iperf_accept(test) < 0) {
                    fprintf(stderr, "iperf_accept: error accepting control socket. exiting...\n");
                    exit(1);
                } 
                FD_CLR(test->listener, &temp_read_set);
            }
            if (FD_ISSET(test->ctrl_sck, &temp_read_set)) {
                // Handle control messages
                iperf_handle_message_server(test);
                FD_CLR(test->ctrl_sck, &temp_read_set);                
            }
            
            if (test->state == CREATE_STREAMS) {
                if (FD_ISSET(test->prot_listener, &temp_read_set)) {
                    // Spawn new streams
                    // XXX: This works, but should it check cookies for authenticity
                    //      Also, currently it uses 5202 for stream connections.
                    //      Should this be fixed to use 5201 instead?
                    iperf_tcp_accept(test);
                    ++streams_accepted;
                    FD_CLR(test->prot_listener, &temp_read_set);
                }
                if (streams_accepted == test->num_streams) {
                    FD_CLR(test->prot_listener, &test->read_set);
                    close(test->prot_listener);
                    test->state = TEST_START;
                    if (write(test->ctrl_sck, &test->state, sizeof(char)) < 0) {
                        perror("write TEST_START");
                        return -1;
                    }
                }
            }
            
            if (test->reverse) {
                // Reverse mode. Server sends.
                iperf_send(test);
            } else {
                // Regular mode. Server receives.
                iperf_recv(test);
            }
        }
    }

    /* Clean up the last test */
    iperf_test_reset(test);
    printf("\n");

    }

    // XXX: Need to put the above while loop into another control structure
    //      that initiates a new test upon each connection rather than dying.

    return 0;
}

