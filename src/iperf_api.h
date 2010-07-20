
/*
   Copyright (c) 2009, The Regents of the University of California, through
   Lawrence Berkeley National Laboratory (subject to receipt of any required
   approvals from the U.S. Dept. of Energy).  All rights reserved.
*/

#ifndef        __IPERF_API_H
#define        __IPERF_API_H

#include "iperf.h"

/**
 * exchange_parameters - handles the param_Exchange part for client
 *
 */
int      iperf_exchange_parameters(struct iperf_test * test);

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
 */
void     iperf_stats_callback(struct iperf_test * test);


/**
 * iperf_reporter_callback -- handles the report printing
 *
 */
void     iperf_reporter_callback(struct iperf_test * test);


/**
 * iperf_run_client -- Runs the client portion of a test
 *
 */
int      iperf_run_client(struct iperf_test * test);

/**
 * iperf_new_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_new_test();

void      iperf_defaults(struct iperf_test * testp);


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
 */
void      iperf_add_stream(struct iperf_test * test, struct iperf_stream * stream);

/**
 * iperf_init_stream -- init resources associated with test
 *
 */
int       iperf_init_stream(struct iperf_stream * sp, struct iperf_test * testp);

/**
 * iperf_free_stream -- free resources associated with test
 *
 */
void      iperf_free_stream(struct iperf_stream * sp);

void get_tcpinfo(struct iperf_test *test, struct iperf_interval_results *rp);
void print_tcpinfo(struct iperf_interval_results *);
void build_tcpinfo_message(struct iperf_interval_results *r, char *message);
void print_interval_results(struct iperf_test * test, struct iperf_stream *sp);
int iperf_connect(struct iperf_test *);
int iperf_client_end(struct iperf_test *);
int iperf_send(struct iperf_test *);
int iperf_recv(struct iperf_test *);
void sig_handler(int);
void usage();
void usage_long();
int all_data_sent(struct iperf_test *);
int package_parameters(struct iperf_test *);
int parse_parameters(struct iperf_test *);
int iperf_create_streams(struct iperf_test *);
int iperf_handle_message_client(struct iperf_test *);
int iperf_exchange_results(struct iperf_test *);
int parse_results(struct iperf_test *, char *);
int iperf_init_test(struct iperf_test *);
int iperf_parse_arguments(struct iperf_test *, int, char **);

#endif

