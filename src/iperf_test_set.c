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

	/*printf("options : %s\n", str);*/

	char **argvs = NULL;	//array of args
	char *tmp = strtok(str, " ");
	int count = 1;

	while (tmp)
	{
		argvs = realloc(argvs, sizeof(char *) * ++count);

		if (argvs == NULL)
			return -1;
		argvs[count - 1] = tmp;

		tmp = strtok(NULL, " ");
	}

	tu->argcs = count;
	tu->argvs = argvs;
	return 0;
}

int 
ts_run_test(struct test_unit* tu, struct iperf_test* main_test)
{
	struct iperf_test *child_test;
	int i;

	if (!main_test->json_output)
		printf("Case %s started \n", tu->test_name);

	if (tu->description)
		if (!main_test->json_output)
			printf("description: \"%s\" \n", tu->description);

	if (!tu->test_count)
	{
		if (!main_test->json_output)
			printf("Case is disabled\n");
		return 0;
	}

	tu->unit_tests = malloc(sizeof(struct iperf_test*) * tu->test_count);

	tu->test_err = malloc(sizeof(int) * tu->test_count);

	for (i = 0; i < tu->test_count; ++i)
		tu->test_err[i] = 0;

	if (ts_parse_args(tu))
		return -1;

	for (i = 0; i < tu->test_count; ++i)
	{
		child_test = iperf_new_test();

		if (!child_test)
		{
			tu->test_err[i] = i_errno;
			tu->unit_tests[i] = NULL;
			continue;
		}
		iperf_defaults(child_test);	/* sets defaults */

		iperf_set_test_role(child_test, 'c');
		iperf_set_test_server_hostname(child_test, main_test->server_hostname);
		iperf_set_test_server_port(child_test, main_test->server_port);

		if (main_test->json_output)
			iperf_set_test_json_output(child_test, 1);

		if (main_test->debug)
			child_test->debug = 1;

		iperf_parse_arguments(child_test, tu->argcs, tu->argvs);

		if (iperf_run_client(child_test) < 0)
			tu->test_err[i] = i_errno;
			//iperf_errexit(child_test, "error - %s", iperf_strerror(i_errno));

		tu->unit_tests[i] = child_test;
	}

	if (!main_test->json_output)
		printf("Case %s finished. \n \n", tu->test_name);

	return 0;
}

int 
ts_run_bulk_test(struct iperf_test* test)
{
	struct test_set* t_set = ts_new_test_set(test->test_set_file);
	int i;

	if(!test->json_output)
		printf("Case count : %d \n", t_set->unit_count);


	for (i = 0; i < t_set->unit_count; ++i)
	{
		if (ts_run_test(t_set->suite[i], test))
			return -1;
	}

	if (!test->json_output)
		ts_err_printf(t_set);

	ts_free_test_set(t_set);

	return 0;
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
	cJSON *tmp_node;

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

	t_set->unit_count = i;

	//parsing
	t_set->suite = malloc(sizeof(struct test_unit*) * i);

	node = json->child;

	for (i = 0; i < t_set->unit_count; ++i)
	{
		struct test_unit* unit = malloc(sizeof(struct test_unit));
		unit->id = i;
		unit->test_name = node->string;
		unit->json_test_case = node;

		tmp_node = cJSON_GetObjectItem(node, "description");
		if (tmp_node)
			unit->description = tmp_node->valuestring;
		else
			unit->description = NULL;

		tmp_node = cJSON_GetObjectItem(node, "repeat");
		if (tmp_node)
		{
			if (tmp_node->valuedouble > 0)
				unit->test_count = tmp_node->valuedouble;
			else
				unit->test_count = 0;
		}
		else
			unit->test_count = 1;

		tmp_node = cJSON_GetObjectItem(node, "active");
		if (tmp_node)
		{
			if (tmp_node->type == cJSON_False)
				unit->test_count = 0;
		}

		t_set->suite[i] = unit;
		node = node->next;
	}

	return t_set;
}

int 
ts_free_test_set(struct test_set* t_set)
{
	int i;
	int j;
	struct test_unit * tmp_unit;


	for (i = 0; i < t_set->unit_count; ++i)
	{
		tmp_unit = t_set->suite[i];

		for (j = 0; j < tmp_unit->test_count; ++j)
		{
			if (tmp_unit->unit_tests[j])
				iperf_free_test(tmp_unit->unit_tests[j]);
		}

		if (tmp_unit->test_count)
		{
			free(tmp_unit->argvs);
			free(tmp_unit->unit_tests);
			free(tmp_unit->test_err);
		}

		free(tmp_unit);
	}

	free(t_set->suite);
	cJSON_Delete(t_set->json_file);
	free(t_set);

	return 0;
}

int
ts_err_printf(struct test_set* t_set)
{
	int case_error;
	int i;
	int j;
	struct test_unit * tmp_unit;

	printf("\n- - - - - - - - - - - - - - - - - - - - - - - - -\n");
	printf("Errors output: \n");

	for (i = 0; i < t_set->unit_count; ++i)
	{
		case_error = 0;

		tmp_unit = t_set->suite[i];
		printf("Case %s:", tmp_unit->test_name);

		for (j = 0; j < tmp_unit->test_count; ++j)
		{
			if (tmp_unit->test_err[j])
			{
				case_error = tmp_unit->test_err[j];
				printf("\n	run %d: error - %s", j, iperf_strerror(i_errno));
			}
		}

		if (!case_error)
			printf(" OK\n");
		else
			printf("\n");
	}

	return 0;
}
