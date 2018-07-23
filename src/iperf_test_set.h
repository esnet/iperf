#ifndef __IPERF_TEST_SET_H 
#define __IPERF_TEST_SET_H

#include "cjson.h"
#include "iperf.h"

#define OPT_TEST_SET 101

struct iperf_test;

/* Structure for containing test's settings*/
struct test_unit
{
    int       id;                    // id, set automatic
    cJSON     *json_test_case;       // JSON contained test
    char      *test_name;            // name from JSON
    char      *description;          // descriptions from JSON

    int       argcs;                 // argc of test
    char      **argvs;               // argv of test

    int       test_count;            // repeat from JSON
    struct iperf_test **unit_tests;  // iperf_test for each repeat
    int       *test_err;             // errors of each tests

    cJSON     *avaraged_results;     // averaging
};

struct test_set
{
    int unit_count;                  // units count from JSON file
    char *path;                      // path of JSON file
    cJSON *json_file;                // file in JSON format
    struct test_unit **suite;        // test_unit container
};

struct benchmark_coefs
{
	// TCP
	double bps_sent;          // bits per second sent
	double bps_received;      // bits per second received
	double max_retransmits;   // max retransmits per second
	double retransmits_coef;  // retransmits coefficient
	/* If max_retransmits > retransmits_per_second then
	 * bench_score += (max_retransmits - retransmits_per_second) * retransmits_coef
	 */

	// UDP
	double bps;               // bits per second
	double jitter_coef;       // jitter coefficient
	double packets_coef;      // packets coefficient
	double max_lost_percent;  // max lost percent
	double lost_percent_coef; // lost percent coefficient
	/* If max_lost_percent > lost_percent then
	 * bench_score += (max_lost_percent - lost_percent then) * lost_percent_coef
	 */
};

int ts_parse_args(struct test_unit* tu);

int ts_run_test(struct test_unit* tu, struct iperf_test* main_test);

int ts_run_bulk_test(struct iperf_test* test);

struct test_unit * ts_new_test_unit(int id, cJSON* node, cJSON* def);

struct test_set * ts_new_test_set(char* path);

int ts_free_test_set(struct test_set* t_set);

int ts_err_output(struct test_set* t_set);

int ts_get_averaged(struct test_set* t_set);

int ts_result_averaging(struct test_unit* t_unit, struct benchmark_coefs* b_coefs);

struct benchmark_coefs * ts_get_benchmark_coefs(cJSON* j_coefs);

#endif
