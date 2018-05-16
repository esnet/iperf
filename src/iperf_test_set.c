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
	struct iperf_stream *sp = NULL;
	iperf_size_t bytes_sent, total_sent = 0;
	iperf_size_t bytes_received, total_received = 0;
	double start_time, end_time = 0.0, avg_jitter = 0.0, lost_percent = 0.0;
	double sender_time = 0.0, receiver_time = 0.0;
	double bandwidth;

	cJSON *result = NULL;

	/* print final summary for all intervals */


	for (j = 0; j < t_unit->test_count; ++j)
	{
		test = t_unit->unit_tests[j]; /* !used struct! */

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
			end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
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

				if (sender_time > 0.0) {
					bandwidth = (double)bytes_sent / (double)sender_time;	// !need to change!
				}
				else {
					bandwidth = 0.0;
				}

				if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
					if (test->sender_has_retransmits) {
						/* Sender summary, TCP and SCTP with retransmits. */

						if (test->role == 's' && !test->sender) {
							if (test->verbose)
								iperf_printf(test, report_sender_not_available_format, sp->socket);
						}
						else {
							iperf_printf(test, report_bw_retrans_format, sp->socket, start_time, sender_time, ubuf, nbuf, sp->result->stream_retrans, report_sender);
						}
					}
					else {
						/* Sender summary, TCP and SCTP without retransmits. */

						if (test->role == 's' && !test->sender) {
							if (test->verbose)
								iperf_printf(test, report_sender_not_available_format, sp->socket);
						}
						else {
							iperf_printf(test, report_bw_format, sp->socket, start_time, sender_time, ubuf, nbuf, report_sender);
						}
					}
				}
				else {
					/* Sender summary, UDP. */
					if (sender_packet_count - sp->omitted_packet_count > 0) {
						lost_percent = 100.0 * (sp->cnt_error - sp->omitted_cnt_error) / (sender_packet_count - sp->omitted_packet_count);
					}
					else {
						lost_percent = 0.0;
					}
					if (test->json_output) {
						/*
						* For hysterical raisins, we only emit one JSON
						* object for the UDP summary, and it contains
						* information for both the sender and receiver
						* side.
						*
						* The JSON format as currently defined only includes one
						* value for the number of packets.  We usually want that
						* to be the sender's value (how many packets were sent
						* by the sender).  However this value might not be
						* available on the receiver in certain circumstances
						* specifically on the server side for a normal test or
						* the client side for a reverse-mode test.  If this
						* is the case, then use the receiver's count of packets
						* instead.
						*/
						int packet_count = sender_packet_count ? sender_packet_count : receiver_packet_count;
						cJSON_AddItemToObject(json_summary_stream, "udp", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f  out_of_order: %d", (int64_t)sp->socket, (double)start_time, (double)sender_time, (double)sender_time, (int64_t)bytes_sent, bandwidth * 8, (double)sp->jitter * 1000.0, (int64_t)(sp->cnt_error - sp->omitted_cnt_error), (int64_t)(packet_count - sp->omitted_packet_count), (double)lost_percent, (int64_t)(sp->outoforder_packets - sp->omitted_outoforder_packets)));
					}
					else {
						/*
						* Due to ordering of messages on the control channel,
						* the server cannot report on client-side summary
						* statistics.  If we're the server, omit one set of
						* summary statistics to avoid giving meaningless
						* results.
						*/
						if (test->role == 's' && !test->sender) {
							if (test->verbose)
								iperf_printf(test, report_sender_not_available_format, sp->socket);
						}
						else {
							iperf_printf(test, report_bw_udp_format, sp->socket, start_time, sender_time, ubuf, nbuf, 0.0, 0, (sender_packet_count - sp->omitted_packet_count), (double)0, report_sender);
						}
						if ((sp->outoforder_packets - sp->omitted_outoforder_packets) > 0)
							iperf_printf(test, report_sum_outoforder, start_time, sender_time, (sp->outoforder_packets - sp->omitted_outoforder_packets));
					}
				}



				/* !Is it necessary that below?! */

				unit_snprintf(ubuf, UNIT_LEN, (double)bytes_received, 'A');
				if (receiver_time > 0) {
					bandwidth = (double)bytes_received / (double)receiver_time;
				}
				else {
					bandwidth = 0.0;
				}


				/* !total information counting! */

				unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
				if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
					/* Receiver summary, TCP and SCTP */
					if (test->json_output)
						cJSON_AddItemToObject(json_summary_stream, "receiver", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f", (int64_t)sp->socket, (double)start_time, (double)receiver_time, (double)end_time, (int64_t)bytes_received, bandwidth * 8));
					else
						if (test->role == 's' && test->sender) {
							if (test->verbose)
								iperf_printf(test, report_receiver_not_available_format, sp->socket);
						}
						else {
							iperf_printf(test, report_bw_format, sp->socket, start_time, receiver_time, ubuf, nbuf, report_receiver);
						}
				}
				else {
					/*
					* Receiver summary, UDP.  Note that JSON was emitted with
					* the sender summary, so we only deal with human-readable
					* data here.
					*/
					if (!test->json_output) {
						if (receiver_packet_count - sp->omitted_packet_count > 0) {
							lost_percent = 100.0 * (sp->cnt_error - sp->omitted_cnt_error) / (receiver_packet_count - sp->omitted_packet_count);
						}
						else {
							lost_percent = 0.0;
						}

						if (test->role == 's' && test->sender) {
							if (test->verbose)
								iperf_printf(test, report_receiver_not_available_format, sp->socket);
						}
						else {
							iperf_printf(test, report_bw_udp_format, sp->socket, start_time, receiver_time, ubuf, nbuf, sp->jitter * 1000.0, (sp->cnt_error - sp->omitted_cnt_error), (receiver_packet_count - sp->omitted_packet_count), lost_percent, report_receiver);
						}
					}
				}
			}
		}
	}
	
	/*
	* This part is creation cJSON object for
	* further use.
	*/

	if (test->protocol->id == Ptcp)
	{

	}
	else // Pudp
	{

	}

	t_unit->avaraged_results = result;

	return 0;
}
