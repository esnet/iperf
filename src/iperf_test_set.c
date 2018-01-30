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
	char *str = options->valuestring;
	printf("options : %s\n", str);
	char **argvs = NULL;
	char *tmp = strtok(str, " ");
	//printf("parsing \n");
	int count = 1, i = 0;

	while (tmp)
	{
		argvs = realloc(argvs, sizeof(char *) * ++count);

		if (argvs == NULL)
			exit(-1);
		argvs[count - 1] = tmp;

		tmp = strtok(NULL, " ");
	}

	for (i = 1; i < (count); ++i)	//debug
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
	//iperf_set_test_json_output(child_test, 1);

	printf("Test %d started \n", tu->id); //add name

	ts_parse_args(tu);

	iperf_parse_arguments(child_test, tu->argcs, tu->argvs);

	if (iperf_run_client(child_test) < 0)
		iperf_errexit(child_test, "error - %s", iperf_strerror(i_errno));

	if (iperf_get_test_json_output_string(child_test)) {
		//printf("%s\n", iperf_get_test_json_output_string(child_test));
	}

	tu->current_test = child_test;

	//iperf_free_test(child_test);

	printf("Finished \n");
	return 0;
}

int 
ts_run_bulk_test(struct iperf_test* test)
{
	struct test_set* t_set = ts_new_test_set(test->test_set_file);
	int i;
	

	for (i = 0; i < t_set->test_count; ++i)
	{
		ts_run_test(t_set->suite[i], test);
	}

	return 0; //add correct completion of the test to the errors(?)
}

struct test_set *
ts_new_test_set(char* path)
{
	struct test_set* t_set = malloc(sizeof(struct test_set));
	int i;
	long size = 0;
	char *str;
	FILE * inputFile = fopen(path, "r");
	cJSON *json;
	cJSON *node;

	if (!inputFile)
	{
		printf("File is not exist");
		return NULL;
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
	t_set->json_file = json;

	fclose(inputFile);
	free(str);


	//test counting
	node = json->child;

	i = 0;

	while (node && cJSON_GetObjectItem(node, "options"))
	{
		++i;
		node = node->next;
	}

	t_set->test_count = i;


	//if (test->debug)
	//	printf("%s\n", cJSON_Print(json));

	printf("Test count : %d \n", i);

	//parsing
	t_set->suite = malloc(sizeof(struct test_unit*) * i);

	node = json->child;

	for (i = 0; i < t_set->test_count; ++i)
	{
		struct test_unit* unit = malloc(sizeof(struct test_unit));
		unit->id = i;
		unit->json_test_case = node;
		t_set->suite[i] = unit;
		node = node->next;
	}

	return t_set;
}

int 
ts_free_test_unit(struct test_set* t_set)
{
	int i;
	/*delete argvs*/

	for (i = 0; i < t_set->test_count; ++i)
	{
		free(t_set->suite[i]);
	}

	free(t_set->suite);
	cJSON_Delete(t_set->json_file);
	free(t_set);

	return 0;
}
