//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


#include "iperf_test_set.h"
#include "iperf_api.h"


int
ts_parse_file(struct test_set &ts)
{
	return 0;
}

int 
ts_run_test(test_unit & tu)
{
	char* host;
	int port;
	struct iperf_test *child_test;

	//pid_t pid;
	//int *shared; /* pointer to the shm */
	//int shmid;

	//shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);

	iperf_set_test_role(child_test, 'c'); //take from parent's process or add to argv
	iperf_set_test_server_hostname(child_test, host);
	iperf_set_test_server_port(child_test, port);

	iperf_parse_arguments(child_test, tu.op_count, tu.args);

	if (iperf_run_client(child_test) < 0) {
		fprintf(stderr, "%s: error - %s\n", argv0, iperf_strerror(i_errno));
		exit(EXIT_FAILURE);
	}

	if (iperf_get_test_json_output_string(child_test)) {
		fprintf(iperf_get_test_outfile(child_test), "%zd bytes of JSON emitted\n",
			strlen(iperf_get_test_json_output_string(child_test)));
	}
	return 0;
}

int 
ts_run_bulk_test(const struct iperf_test *test)
{
	struct test_set t_set;
	int i;
	long size = 0;
	char *str;
	FILE * inputFile = fopen(test->test_set_file, "r");
	cJSON json;

	if (!inputFile)
		return -1;
	else
	{
		fseek(inputFile, 0, SEEK_END);
		size = ftell(inputFile);
		fseek(inputFile, 0, SEEK_SET);
	}

	str = malloc(size + 1);
	fread(str, size, 1, inputFile);
	str[size] = '\n';

	json = cJSON_Parse(str);

	pid_t pid;
	int *shared; /* pointer to the shm */

	t_set.res = 0;
	t_set.test_count = 0;
	t_set.path = strdup(path);

	if (ts_parse_file(t_set) == -1)
		return -1;
	
	for (i = 0; i < t_set.test_count; ++i)
	{
		ts_run_test(t_set.suite[i]);
	}

	free(t_set.path);
	return 0; //add correct completion of the test to the errors(?)
}