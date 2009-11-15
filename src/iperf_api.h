
/*
   Copyright (c) 2009, The Regents of the University of California, through
   Lawrence Berkeley National Laboratory (subject to receipt of any required
   approvals from the U.S. Dept. of Energy).  All rights reserved.
*/

#ifndef        IPERF_API_H
#define        IPERF_API_H

#include "iperf.h"

/**
 * exchange_parameters - handles the param_Exchange part for client
 *
 */
void      exchange_parameters(struct iperf_test * test);

/**
 * add_to_interval_list -- adds new interval to the interval_list
 *
 */
void      add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results *temp);

/**
 * Display -- Displays interval results for test
 * Mainly for DEBUG purpose
 *
 */
void      display_interval_list(struct iperf_stream_result * rp, int tflag);

/**
 * send_result_to_client - sends result to client via
 *  a new TCP connection
 */
void      send_result_to_client(struct iperf_stream * sp);

/**
 * receive_result_from_server - Receives result from server via
 *  a new TCP connection
 */
void      receive_result_from_server(struct iperf_test * test);

/**
 * connect_msg -- displays connection message
 * denoting senfer/receiver details
 *
 */
void      connect_msg(struct iperf_stream * sp);



/**
 * Display -- Displays streams in a test
 * Mainly for DEBUG purpose
 *
 */
void      Display(struct iperf_test * test);


/**
 * iperf_stats_callback -- handles the statistic gathering
 *
 *returns void *
 *
 */
void     *iperf_stats_callback(struct iperf_test * test);


/**
 * iperf_reporter_callback -- handles the report printing
 *
 *returns report
 *
 */
char     *iperf_reporter_callback(struct iperf_test * test);


/**
 * iperf_run_client -- Runs the client portion of a test
 *
 */
void      iperf_run_client(struct iperf_test * test);

/**
 * iperf_new_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_new_test();

void      iperf_defaults(struct iperf_test * testp);

/**
 * iperf_init_test -- perform pretest initialization (listen on sockets, etc)
 *
 */
void      iperf_init_test(struct iperf_test * testp);

/**
 * iperf_free_test -- free resources used by test, calls iperf_free_stream to
 * free streams
 *
 */
void      iperf_free_test(struct iperf_test * testp);


/**
 * iperf_new_stream -- return a net iperf_stream with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_stream *iperf_new_stream(struct iperf_test * testp);

struct iperf_stream *iperf_new_udp_stream(struct iperf_test * testp);

/**
 * iperf_add_stream -- add a stream to a test
 *
 * returns 1 on success 0 on failure
 */
int       iperf_add_stream(struct iperf_test * test, struct iperf_stream * stream);

/**
 * iperf_init_stream -- init resources associated with test
 *
 */
void      iperf_init_stream(struct iperf_stream * sp, struct iperf_test * testp);

/**
 * iperf_free_stream -- free resources associated with test
 *
 */
void      iperf_free_stream(struct iperf_stream * sp);

void get_tcpinfo(struct iperf_test *test, struct iperf_interval_results *rp);
void print_tcpinfo(struct iperf_interval_results *);
void build_tcpinfo_message(struct iperf_interval_results *r, char *message);
void safe_strcat(char *s1, char *s2);
char * print_interval_results(struct iperf_test * test, struct iperf_stream *sp, char *m);



#endif  /* IPERF_API_H */

