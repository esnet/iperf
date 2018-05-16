//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

//#include <sys/socket.h>

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
		ts_err_output(t_set);

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

	// creating json file
	str = malloc(size + 1);
	fread(str, size, 1, inputFile);
	str[size] = '\n';

	json = cJSON_Parse(str);
	t_set->json_file = json;

	fclose(inputFile);
	free(str);


	// test counting
	node = json->child;

	i = 0;

	while (node && cJSON_GetObjectItem(node, "options"))
	{
		++i;
		node = node->next;
	}

	t_set->unit_count = i;

	// parsing
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

	for (i = 0; i < t_set->unit_count; ++i)
	{
		ts_result_averaging(t_set->suite[i]);
		// if (json ...)
	}

	return 0;
}

int
ts_result_averaging(struct test_unit* t_unit)
{
	int j;

	struct iperf_test* test;

	cJSON *json_summary_stream = NULL;
	int total_retransmits = 0;
	int total_packets = 0, lost_packets = 0;
	int sender_packet_count = 0, receiver_packet_count = 0; /* for this stream, this interval */
	int sender_total_packets = 0, receiver_total_packets = 0; /* running total */
	char ubuf[UNIT_LEN];
	char nbuf[UNIT_LEN];
	char sbuf[UNIT_LEN];
	struct iperf_stream *sp = NULL;
	iperf_size_t bytes_sent, total_sent = 0;
	iperf_size_t bytes_received, total_received = 0;
	double start_time, end_time = 0.0, avg_jitter = 0.0, lost_percent = 0.0;
	double sender_time = 0.0, receiver_time = 0.0;
	double bandwidth;

	cJSON *result = cJSON_CreateObject();

	/* print final summary for all intervals */


	for (j = 0; j < t_unit->test_count; ++j)
	{
		test = t_unit->unit_tests[j]; /* !used struct! */

		/* print final summary for all intervals */

		start_time = 0.;
		sp = SLIST_FIRST(&test->streams);
		/*
		* If there is at least one stream, then figure out the length of time
		* we were running the tests and print out some statistics about
		* the streams.  It's possible to not have any streams at all
		* if the client got interrupted before it got to do anything.
		*
		* Also note that we try to keep seperate values for the sender
		* and receiver ending times.  Earlier iperf (3.1 and earlier)
		* servers didn't send that to the clients, so in this case we fall
		* back to using the client's ending timestamp.  The fallback is
		* basically emulating what iperf 3.1 did.
		*/
		if (sp) {
			end_time += timeval_diff(&sp->result->start_time, &sp->result->end_time);
			if (test->sender) {
				sp->result->sender_time = end_time;
				if (sp->result->receiver_time == 0.0) {
					sp->result->receiver_time = sp->result->sender_time;
				}
			}
			else {
				sp->result->receiver_time = end_time;
				if (sp->result->sender_time == 0.0) {
					sp->result->sender_time = sp->result->receiver_time;
				}
			}
			sender_time = sp->result->sender_time;
			receiver_time = sp->result->receiver_time;
			SLIST_FOREACH(sp, &test->streams, streams) {

				bytes_sent = sp->result->bytes_sent - sp->result->bytes_sent_omit;
				bytes_received = sp->result->bytes_received;
				total_sent += bytes_sent;
				total_received += bytes_received;

				if (test->sender) {
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
					/*
					* Running total of the total number of packets.  Use the sender packet count if we
					* have it, otherwise use the receiver packet count.
					*/
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
	

	//!!

	if (test->num_streams > 1 || test->json_output) {
		cJSON *obj = cJSON_CreateObject();
		/* If no tests were run, arbitrarily set bandwidth to 0. */
		if (sender_time > 0.0) {
			bandwidth = (double)total_sent / (double)sender_time;
		}
		else {
			bandwidth = 0.0;
		}

		cJSON_AddItemToObject(obj, "start_time", "0.");

		cJSON_AddItemToObject(result, "sum_send", obj);
		unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
		if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
			if (test->sender_has_retransmits) {
				/* Summary sum, TCP with retransmits. */
				if (test->json_output)
					cJSON_AddItemToObject(test->json_end, "sum_sent", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  retransmits: %d", (double)start_time, (double)sender_time, (double)sender_time, (int64_t)total_sent, bandwidth * 8, (int64_t)total_retransmits));
				else
					if (test->role == 's' && !test->sender) {
						if (test->verbose)
							iperf_printf(test, report_sender_not_available_format, sp->socket);
					}
					else {
						iperf_printf(test, report_sum_bw_retrans_format, start_time, sender_time, ubuf, nbuf, total_retransmits, report_sender);
					}
			}

			/* If no tests were run, set received bandwidth to 0 */
			if (receiver_time > 0.0) {
				bandwidth = (double)total_received / (double)receiver_time;
			}
			else {
				bandwidth = 0.0;
			}
			unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
			if (test->json_output)
				cJSON_AddItemToObject(test->json_end, "sum_received", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f", (double)start_time, (double)receiver_time, (double)receiver_time, (int64_t)total_received, bandwidth * 8));
			else
				if (test->role == 's' && test->sender) {
					if (test->verbose)
						iperf_printf(test, report_receiver_not_available_format, sp->socket);
				}
				else {
					iperf_printf(test, report_sum_bw_format, start_time, receiver_time, ubuf, nbuf, report_receiver);
				}
		}
		else {
			/* Summary sum, UDP. */
			avg_jitter /= test->num_streams; /* !fix it! */
			/* If no packets were sent, arbitrarily set loss percentage to 0. */
			if (total_packets > 0) {
				lost_percent = 100.0 * lost_packets / total_packets;
			}
			else {
				lost_percent = 0.0;
			}
			if (test->json_output)
				cJSON_AddItemToObject(test->json_end, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f", (double)start_time, (double)receiver_time, (double)receiver_time, (int64_t)total_sent, bandwidth * 8, (double)avg_jitter * 1000.0, (int64_t)lost_packets, (int64_t)total_packets, (double)lost_percent));
			else {
				/*
				* On the client we have both sender and receiver overall summary
				* stats.  On the server we have only the side that was on the
				* server.  Output whatever we have.
				*/
				if (!(test->role == 's' && !test->sender)) {
					unit_snprintf(ubuf, UNIT_LEN, (double)total_sent, 'A');
					iperf_printf(test, report_sum_bw_udp_format, start_time, sender_time, ubuf, nbuf, 0.0, 0, sender_total_packets, 0.0, "sender");
				}
				if (!(test->role == 's' && test->sender)) {

					unit_snprintf(ubuf, UNIT_LEN, (double)total_received, 'A');
					/* Compute received bandwidth. */
					if (end_time > 0.0) {
						bandwidth = (double)total_received / (double)receiver_time;
					}
					else {
						bandwidth = 0.0;
					}
					unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
					iperf_printf(test, report_sum_bw_udp_format, start_time, receiver_time, ubuf, nbuf, avg_jitter * 1000.0, lost_packets, receiver_total_packets, lost_percent, "receiver");
				}
			}
		}
	}


	/* Is it need? 
	if (test->json_output) { 
		cJSON_AddItemToObject(test->json_end, "cpu_utilization_percent", iperf_json_printf("host_total: %f  host_user: %f  host_system: %f  remote_total: %f  remote_user: %f  remote_system: %f", (double)test->cpu_util[0], (double)test->cpu_util[1], (double)test->cpu_util[2], (double)test->remote_cpu_util[0], (double)test->remote_cpu_util[1], (double)test->remote_cpu_util[2]));
		if (test->protocol->id == Ptcp) {
			char *snd_congestion = NULL, *rcv_congestion = NULL;
			if (test->sender) {
				snd_congestion = test->congestion_used;
				rcv_congestion = test->remote_congestion_used;
			}
			else {
				snd_congestion = test->remote_congestion_used;
				rcv_congestion = test->congestion_used;
			}
			if (snd_congestion) {
				cJSON_AddStringToObject(test->json_end, "sender_tcp_congestion", snd_congestion);
			}
			if (rcv_congestion) {
				cJSON_AddStringToObject(test->json_end, "receiver_tcp_congestion", rcv_congestion);
			}
		}
	}
	*/


	//!!

	/*
	* This part is creation cJSON object for
	* further use.
	*/



	if (test->protocol->id == Ptcp)
	{
		cJSON_AddItemToObject(result, "protocol", "TCP");
	}
	else // UDP
	{
		cJSON_AddItemToObject(result, "protocol", "UDP");
	}

	t_unit->avaraged_results = result;

	return 0;
}
