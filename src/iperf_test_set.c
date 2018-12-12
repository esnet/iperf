#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "iperf_locale.h"
#include "iperf_test_set.h"
#include "iperf_api.h"
#include "iperf_util.h"

#include "iperf_tcp.h"
#include "iperf_udp.h"

int
ts_parse_args(struct test_unit* tu)
{
	cJSON* options = cJSON_GetObjectItemCaseSensitive(tu->json_test_case, "options");
	char *str = options->valuestring;

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
	int repeated = 0;

	if (!main_test->json_output)
	{
		printf("\n- - - - - - - - - - - - - - - - - - - - - - - - -\n");
		printf("Case %s started \n", tu->test_name);
	}

	if (tu->description)
		if (!main_test->json_output)
			printf("Description: \"%s\"\n", tu->description);

	if (!tu->test_count)
	{
		if (!main_test->json_output)
			printf("Case disabled\n\n");
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

		/*Copying settings from main_test*/
		iperf_set_test_role(child_test, 'c');
		iperf_set_test_server_hostname(child_test, main_test->server_hostname);
		iperf_set_test_server_port(child_test, main_test->server_port);

		if (main_test->json_output)
			iperf_set_test_json_output(child_test, 1);

		if (main_test->debug)
			child_test->debug = 1;


		iperf_parse_arguments(child_test, tu->argcs, tu->argvs);

		/* Running test*/
		if (iperf_run_client(child_test) < 0)
		{
			/* If test was failed we restart it one time*/
			if (!repeated)
			{
				--i;
				repeated = 1;
				iperf_free_test(child_test);
				usleep(500000);
				continue;
			}
			else
			{
				repeated = 0;
				tu->test_err[i] = i_errno;
				if (!child_test->json_output)
					printf("\n");
			}
		}
		else
		{
			repeated = 0;
			if (!child_test->json_output)
				printf("\n");
		}

		tu->unit_tests[i] = child_test;

		/* Sleep between tests */
		usleep(10000);
	}

	if (!main_test->json_output)
		printf("Case %s finished.\n\n", tu->test_name);

	return 0;
}

int 
ts_run_bulk_test(struct iperf_test* test)
{

	struct test_set* t_set = ts_new_test_set(test->test_set_file);
	int i;

	if (!t_set)
		return -1;

	if(!test->json_output)
		printf("Case count : %d \n", t_set->unit_count);


	for (i = 0; i < t_set->unit_count; ++i)
	{
		if (ts_run_test(t_set->suite[i], test))
			return -1;
	}

	ts_get_averaged(t_set);

	if (!test->json_output)
		ts_err_output(t_set);

	ts_free_test_set(t_set);

	return 0;
}

struct test_unit *
ts_new_test_unit(int id, cJSON* node, cJSON* def)
{
	cJSON *tmp_node;
	cJSON *tmp_def;

	struct test_unit* unit = malloc(sizeof(struct test_unit));
	unit->id = id;
	unit->test_name = node->string;
	unit->json_test_case = node;

	tmp_node = cJSON_GetObjectItem(node, "description");
	if (tmp_node)
		unit->description = tmp_node->valuestring;
	else
		unit->description = NULL;

	tmp_node = cJSON_GetObjectItem(node, "repeat");
	tmp_def = cJSON_GetObjectItem(def, "repeat");
	if (tmp_node)
	{
		if (tmp_node->valuedouble > 0)
			unit->test_count = tmp_node->valuedouble;
		else
			unit->test_count = 0;
	}
	else {
		if (tmp_def)
		{
			if (tmp_def->valuedouble > 0)
						unit->test_count = tmp_def->valuedouble;
					else
						unit->test_count = 0;
		}
		else
			unit->test_count = 1;
	}


	tmp_node = cJSON_GetObjectItem(node, "active");
	tmp_def = cJSON_GetObjectItem(def, "active");
	if (tmp_node)
	{
		if (tmp_node->type == cJSON_False)
			unit->test_count = 0;
	}
	else
	{
		if (tmp_def)
			if (tmp_def->type == cJSON_False)
					unit->test_count = 0;
	}

	return unit;
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
	cJSON *def;

	if (!inputFile)
	{
		fprintf(stderr, "File \"%s\" is not exist.\n", path);
		return NULL;
	}
	else
	{
		fseek(inputFile, 0, SEEK_END);
		size = ftell(inputFile);
		fseek(inputFile, 0, SEEK_SET);
	}


	/* Parse input file to JSON object */
	str = malloc(size + 1);
	if (fread(str, size, 1, inputFile) != 1)
		printf("Error\n");
	str[size] = '\n';

	json = cJSON_Parse(str);
	t_set->json_file = json;

	fclose(inputFile);
	free(str);


	/* Counting nodes with item options.
	 * All nodes without options are ignored.*/
	node = json->child;
	i = 0;

	while (node)
	{
		if (cJSON_GetObjectItem(node, "options"))
			++i;
		node = node->next;
	}
	t_set->unit_count = i;


	/* Parsing JSON file to test_set */
	t_set->suite = malloc(sizeof(struct test_unit*) * i);

	node = json->child;
	i = 0;

	def = cJSON_GetObjectItem(json, "default");

	while (node)
	{
		if (cJSON_GetObjectItem(node, "options"))
		{
			t_set->suite[i] = ts_new_test_unit(i, node, def);
			++i;
		}
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
ts_err_output(struct test_set* t_set)
{
	int i;
	int j;
	int errorIsPrinted = 0;
	struct test_unit * tmp_unit;

	for (i = 0; i < t_set->unit_count; ++i)
	{
		tmp_unit = t_set->suite[i];

		for (j = 0; j < tmp_unit->test_count; ++j)
			if (tmp_unit->test_err[j])
			{
				if (tmp_unit->unit_tests && tmp_unit->unit_tests[0]->outfile) {
					if (!errorIsPrinted)
					{
						fprintf(tmp_unit->unit_tests[0]->outfile, "\n- - - - - - - - - - - - - - - - - - - - - - - - -\n");
						fprintf(tmp_unit->unit_tests[0]->outfile, "Errors output: \n");
						errorIsPrinted = 1;
					}
					fprintf(tmp_unit->unit_tests[0]->outfile, "Case %s:\n", tmp_unit->test_name);
				}
				else {
					if (!errorIsPrinted)
					{
						fprintf(stderr, "\n- - - - - - - - - - - - - - - - - - - - - - - - -\n");
						fprintf(stderr, "Errors output: \n");
						errorIsPrinted = 1;
					}
					fprintf(stderr, "Case %s:\n", tmp_unit->test_name);
				}
				break;
			}

		for (j = 0; j < tmp_unit->test_count; ++j)
		{
			if (tmp_unit->test_err[j])
			{
				if (tmp_unit->unit_tests && tmp_unit->unit_tests[0]->outfile) {
					fprintf(tmp_unit->unit_tests[0]->outfile, "    run %d: error - %s\n", j, iperf_strerror(tmp_unit->test_err[j]));
				}
				else {
					fprintf(stderr, "    run %d: error - %s\n", j, iperf_strerror(tmp_unit->test_err[j]));
				}
			}
		}
	}

	return 0;
}

int
ts_get_averaged(struct test_set* t_set)
{
	int i;
	cJSON *tmp_node;
	cJSON *root = cJSON_CreateArray();
	struct benchmark_coefs* b_coefs = ts_get_benchmark_coefs(cJSON_GetObjectItem(t_set->json_file, "coefficients"));

	for (i = 0; i < t_set->unit_count; ++i)
	{
		tmp_node = cJSON_GetObjectItemCaseSensitive(t_set->suite[i]->json_test_case, "averaging");
		if (tmp_node)
		{
			if (cJSON_IsTrue(tmp_node) && t_set->suite[i]->test_count)
			{
				ts_result_averaging(t_set->suite[i], b_coefs);
				cJSON_AddItemToArray(root, t_set->suite[i]->avaraged_results);

			}
		}
	}

	printf("%s\n",cJSON_Print(root));

	free(b_coefs);

	return 0;
}

int
ts_result_averaging(struct test_unit* t_unit, struct benchmark_coefs* b_coefs)
{
	int j;

	struct iperf_test* test = NULL;

	int total_retransmits = 0;
	int total_packets = 0, lost_packets = 0;
	int sender_packet_count = 0, receiver_packet_count = 0; /* for this stream, this interval */
	int sender_total_packets = 0, receiver_total_packets = 0; /* running total */
	struct iperf_stream *sp = NULL;
	iperf_size_t bytes_sent, total_sent = 0;
	iperf_size_t bytes_received, total_received = 0;
	double start_time, avg_jitter = 0.0, lost_percent = 0.0;
	double sender_time = 0.0, receiver_time = 0.0;
	double bandwidth;

	unsigned long int benchmark = 0;

	cJSON *obj;

	cJSON *value;
	cJSON *result = cJSON_CreateObject();

	cJSON_AddStringToObject(result, "type" , "averaged_result");


	/* Get sum tests result */
	for (j = 0; j < t_unit->test_count; ++j)
	{
		test = t_unit->unit_tests[j];

		start_time = 0.;
		sp = SLIST_FIRST(&test->streams);

		if (sp) {
			sender_time += sp->result->sender_time;
			receiver_time += sp->result->receiver_time;
			SLIST_FOREACH(sp, &test->streams, streams) {

				bytes_sent = sp->result->bytes_sent - sp->result->bytes_sent_omit;
				bytes_received = sp->result->bytes_received;
				total_sent += bytes_sent;
				total_received += bytes_received;

				if (test->mode == SENDER) {
					sender_packet_count = sp->packet_count;
					receiver_packet_count = sp->peer_packet_count;
				}
				else {
					sender_packet_count = sp->peer_packet_count;
					receiver_packet_count = sp->packet_count;
				}

				if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
					if (test->sender_has_retransmits) {
						total_retransmits += sp->result->stream_retrans;
					}
				}
				else {
					int packet_count = sender_packet_count ? sender_packet_count : receiver_packet_count;
					total_packets += (packet_count - sp->omitted_packet_count);
					sender_total_packets += (sender_packet_count - sp->omitted_packet_count);
					receiver_total_packets += (receiver_packet_count - sp->omitted_packet_count);
					lost_packets += (sp->cnt_error - sp->omitted_cnt_error);
					avg_jitter += sp->jitter;
				}
			}
		}
	}


	/*
	* This part is creation cJSON object for
	* further use. */
	value = cJSON_CreateNumber(t_unit->id);
	cJSON_AddItemToObject(result, "id", value);
	
	value = cJSON_CreateString(t_unit->test_name);
	cJSON_AddItemToObject(result, "test_name", value);

	value = cJSON_CreateNumber(t_unit->test_count);
	cJSON_AddItemToObject(result, "repeat", value);

	if (test->protocol->id == Ptcp)
		value = cJSON_CreateString("TCP");
	else // UDP
		value = cJSON_CreateString("UDP");

	cJSON_AddItemToObject(result, "protocol", value);


	/* Build result to JSON */

	/* If no tests were run, arbitrarily set bandwidth to 0. */
	if (sender_time > 0.0) {
		bandwidth = (double)total_sent / (double)sender_time;
	}
	else {
		bandwidth = 0.0;
	}

	if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {

		// sent
		obj = cJSON_CreateObject();

		value = cJSON_CreateNumber(start_time);
		cJSON_AddItemToObject(obj, "start", value);

		value = cJSON_CreateNumber((double)sender_time / t_unit->test_count);
		cJSON_AddItemToObject(obj, "end", value);

		value = cJSON_CreateNumber((double)total_sent / t_unit->test_count);
		cJSON_AddItemToObject(obj, "bytes", value);

		value = cJSON_CreateNumber((double)bandwidth * 8);
		cJSON_AddItemToObject(obj, "bits_per_second", value);

		if (total_retransmits) {
			value = cJSON_CreateNumber(total_retransmits / t_unit->test_count);
			cJSON_AddItemToObject(obj, "retransmits", value);
		}

		cJSON_AddItemToObject(result, "sum_sent(avg)", obj);

		// benchmark
		benchmark += bandwidth * 8 / 1024 / 10 * b_coefs->bps_sent;

		// received
		if (receiver_time > 0.0) {
			bandwidth = (double)total_received / (double)receiver_time;
		}
		else {
			bandwidth = 0.0;
		}

		obj = cJSON_CreateObject();

		value = cJSON_CreateNumber(start_time);
		cJSON_AddItemToObject(obj, "start", value);

		value = cJSON_CreateNumber((double)receiver_time / t_unit->test_count);
		cJSON_AddItemToObject(obj, "end", value);

		value = cJSON_CreateNumber((double)total_received / t_unit->test_count);
		cJSON_AddItemToObject(obj, "bytes", value);

		value = cJSON_CreateNumber((double)bandwidth * 8);
		cJSON_AddItemToObject(obj, "bits_per_second", value);

		cJSON_AddItemToObject(result, "sum_received(avg)", obj);

		// benchmark
		benchmark += bandwidth * 8 / 1024 / 10 * b_coefs->bps_received;
		benchmark /= 2;
		benchmark += (b_coefs->max_retransmits - total_retransmits / sender_time) > 0 ?
				((b_coefs->max_retransmits - total_retransmits / sender_time) * b_coefs->retransmits_coef) : 0;
	}
	else {
		/* Summary sum, UDP. */
		avg_jitter /= test->num_streams;
		/* If no packets were sent, arbitrarily set loss percentage to 0. */
		if (total_packets > 0) {
			lost_percent = 100.0 * lost_packets / total_packets;
		}
		else {
			lost_percent = 0.0;
		}

		obj = cJSON_CreateObject();

		value = cJSON_CreateNumber(start_time);
		cJSON_AddItemToObject(obj, "start", value);

		value = cJSON_CreateNumber((double)receiver_time / t_unit->test_count);
		cJSON_AddItemToObject(obj, "end", value);

		value = cJSON_CreateNumber((double)total_sent / t_unit->test_count);
		cJSON_AddItemToObject(obj, "bytes", value);

		value = cJSON_CreateNumber((double)bandwidth * 8);
		cJSON_AddItemToObject(obj, "bits_per_second", value);

		value = cJSON_CreateNumber((double)avg_jitter * 1000.0 / t_unit->test_count);
		cJSON_AddItemToObject(obj, "jitter_ms", value);

		value = cJSON_CreateNumber((double)lost_packets / t_unit->test_count);
		cJSON_AddItemToObject(obj, "lost_packets", value);

		value = cJSON_CreateNumber((double)total_packets / t_unit->test_count);
		cJSON_AddItemToObject(obj, "packets", value);

		value = cJSON_CreateNumber((double)lost_percent);
		cJSON_AddItemToObject(obj, "lost_percent", value);

		cJSON_AddItemToObject(result, "sum(avg)", obj);

		// benchmark
		benchmark += bandwidth * 8 / 1024 / 10 * b_coefs->bps;
		benchmark += total_packets / receiver_time * b_coefs->packets_coef;
		benchmark += (b_coefs->max_lost_percent - lost_percent) > 0 ?
						(b_coefs->max_lost_percent - lost_percent) * b_coefs->lost_percent_coef : 0;
	}

	if (!(sender_time > 0))
		benchmark = 0;

	value = cJSON_CreateNumber(benchmark);
	cJSON_AddItemToObject(result, "benchmark_score", value);

	t_unit->avaraged_results = result;;

	return 0;
}


struct benchmark_coefs *
ts_get_benchmark_coefs(cJSON* j_coefs)
{
	cJSON* tmp_node;
	struct benchmark_coefs* coefs = malloc(sizeof(struct benchmark_coefs));


	/* Set default */

	// TCP
	coefs->bps_sent = 1;
	coefs->bps_received = 1;
	coefs->max_retransmits = 100;
	coefs->retransmits_coef = 10;


	// UDP
	coefs->bps = 1;
	coefs->jitter_coef = 10;
	coefs->packets_coef = 1;
	coefs->max_lost_percent = 20;
	coefs->lost_percent_coef = 100;

	if (j_coefs)
	{
		/* TCP */
		// bits per second received
		tmp_node = cJSON_GetObjectItem(j_coefs, "bps_received");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->bps_received = tmp_node->valuedouble;

		// bits per second sent
		tmp_node = cJSON_GetObjectItem(j_coefs, "bps_sent");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->bps_sent = tmp_node->valuedouble;

		// max retransmits
		tmp_node = cJSON_GetObjectItem(j_coefs, "max_retransmits");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->max_retransmits = tmp_node->valuedouble;

		// retransmits coefficient
		tmp_node = cJSON_GetObjectItem(j_coefs, "retransmits_coef");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->retransmits_coef = tmp_node->valuedouble;

		/* UDP */
		// bits per second
		tmp_node = cJSON_GetObjectItem(j_coefs, "bps");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->bps = tmp_node->valuedouble;

		// jitter coefficient
		tmp_node = cJSON_GetObjectItem(j_coefs, "jitter_coef");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->jitter_coef = tmp_node->valuedouble;

		// packets coefficient
		tmp_node = cJSON_GetObjectItem(j_coefs, "packets_coef");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->packets_coef = tmp_node->valuedouble;

		// max lost percent
		tmp_node = cJSON_GetObjectItem(j_coefs, "max_lost_percent");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->max_lost_percent = tmp_node->valuedouble;

		// lost percent coefficient
		tmp_node = cJSON_GetObjectItem(j_coefs, "lost_percent_coef");
		if (tmp_node)
			if (tmp_node->valuedouble > 0)
				coefs->lost_percent_coef = tmp_node->valuedouble;
	}

	return coefs;
}
