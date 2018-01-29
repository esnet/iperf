
#ifndef __IPERF_TEST_SET_H 
#define __IPERF_TEST_SET_H

#include "cjson.h"
#include "iperf.h"

#define OPT_TEST_SET 101

struct iperf_test;
    
struct test_unit
{
    int       id;
    int       argcs;
    char      **argvs;
    char      *json_output_string;
    cJSON     *json_test_case;
    cJSON     *json_output;
	struct iperf_test *current_test;
};

struct test_set
{
    int res;
    int test_count;
    char *path;
    struct test_unit **suite;
};

int ts_parse_args(struct test_unit* tu);

int ts_run_test(struct test_unit* tu, struct iperf_test* main_test);

int ts_run_bulk_test(struct iperf_test* test);

int ts_create_tests(struct test_unit* tu);


#endif
