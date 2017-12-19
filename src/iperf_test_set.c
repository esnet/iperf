//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


#include "iperf_test_set.h"
#include "iperf_api.h"


int
ts_parse_args(struct test_unit* tu)
{
	cJSON* options = cJSON_GetObjectItemCaseSensitive(tu->json_test_case, "options");
	char str[] = "-t 10 -i 5";
	char **argvs = NULL;
	char *tmp = strtok(str, " ");
	printf("parsing \n");
	int count = 0, i = 0;

	while (tmp)
	{
		argvs = realloc(argvs, sizeof(char *) * ++count);

		if (argvs == NULL)
			exit(-1);
		argvs[count - 1] = tmp;

		tmp = strtok(NULL, " ");
	}

	for (i = 0; i < (count); ++i)
		printf("res[%d] = %s\n", i, argvs[i]);
	tu->argcs = count;
	tu->argvs = argvs;
	return 0;
}

int 
ts_run_test(struct test_unit* tu, struct iperf_test* main_test)
{
	struct iperf_test *child_test;
	child_test = iperf_new_test();

	if (!child_test)
		iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
	iperf_defaults(child_test);	/* sets defaults */

	iperf_set_test_role(child_test, 'c'); 
	iperf_set_test_server_hostname(child_test, main_test->server_hostname);
	iperf_set_test_server_port(child_test, main_test->server_port);
	iperf_set_test_json_output(child_test, 1);

	ts_parse_args(tu);

	iperf_parse_arguments(child_test, tu->argcs, tu->argvs);

	if (iperf_run_client(child_test) < 0) {
		exit(EXIT_FAILURE);
	}
	else 		printf("Running clint \n");

	if (iperf_get_test_json_output_string(child_test)) {
		printf("%s\n", iperf_get_test_json_output_string(child_test));
	}

	printf("Finished \n");
	return 0;
}

int 
ts_run_bulk_test(struct iperf_test* test)
{
	struct test_set t_set;
	int i;
	long size = 0;
	char *str;
	FILE * inputFile = fopen(test->test_set_file, "r");
	cJSON *json;
	cJSON *node;

	if (!inputFile)
	{ 
		printf("File is not exist");
		return -1;
	}
	else
	{
		fseek(inputFile, 0, SEEK_END);
		size = ftell(inputFile);
		fseek(inputFile, 0, SEEK_SET);
	}

	//creating json file
	str = malloc(size + 1);
	fread(str, size, 1, inputFile);
	str[size] = '\n';

	json = cJSON_Parse(str);

	fclose(inputFile);
	free(str);


	//test counting
	node = json->child;

	i = 0;

	do
	{
		++i;
		node = node->next;
	} while (node);

	t_set.test_count = i;



	printf("%s\n", cJSON_Print(json));

	printf("%d \n", i);

	//parsing
	t_set.suite = malloc(sizeof(struct test_unit*) * i);

	node = json->child;

	for (i = 0; i < t_set.test_count; ++i)
	{
		struct test_unit* unit = malloc(sizeof(struct test_unit));
		unit->id = i;
		unit->json_test_case = node;
		t_set.suite[i] = unit;
		node = node->next;
	}

	for (i = 0; i < t_set.test_count; ++i)
	{
		ts_run_test(t_set.suite[i], test);
	}



	/*delete argvs*/


	cJSON_Delete(json);
	free(t_set.suite);

	return 0; //add correct completion of the test to the errors(?)
}