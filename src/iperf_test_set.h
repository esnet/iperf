
#ifndef __IPERF_TEST_SET_H 
#define __IPERF_TEST_SET_H

#include "cjson.h"

#define OPT_TEST_SET 101
    
struct test_unit
{
    int       id;
	int       op_count;
	char	  **args;
    char      *json_output_string;
    cJSON     *json_test_case;
    cJSON     *json_output;
};

struct test_set
{
	int res;
	int test_count;
	char *path;
	struct test_unit *suite;
};


int ts_parse_file(struct test_set &ts);

int ts_run_test(struct test_unit &tu);

int ts_run_bulk_test(const char* path);


#endif
