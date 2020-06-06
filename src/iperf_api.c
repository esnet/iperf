/*
 * iperf, Copyright (c) 2014-2020, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#define __USE_GNU

#include "iperf_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>

#if defined(HAVE_CPUSET_SETAFFINITY)
#include <sys/param.h>
#include <sys/cpuset.h>
#endif /* HAVE_CPUSET_SETAFFINITY */

#if defined(__CYGWIN__) || defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
#define CPU_SETSIZE __CPU_SETSIZE
#endif /* __CYGWIN__, _WIN32, _WIN64, __WINDOWS__ */

#if defined(HAVE_SETPROCESSAFFINITYMASK)
#include <Windows.h>
#endif /* HAVE_SETPROCESSAFFINITYMASK */

#include "net.h"
#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#if defined(HAVE_SCTP)
#include "iperf_sctp.h"
#endif /* HAVE_SCTP */
#include "timer.h"

#include "cjson.h"
#include "units.h"
#include "iperf_util.h"
#include "iperf_locale.h"
#include "version.h"
#if defined(HAVE_SSL)
#include <openssl/bio.h>
#include "iperf_auth.h"
#endif /* HAVE_SSL */

/* Forwards. */
static int send_parameters(struct iperf_test *test);
static int get_parameters(struct iperf_test *test);
static int send_results(struct iperf_test *test);
static int get_results(struct iperf_test *test);
static int diskfile_send(struct iperf_stream *sp);
static int diskfile_recv(struct iperf_stream *sp);
static int JSON_write(int fd, cJSON *json);
static void print_interval_results(struct iperf_test *test, struct iperf_stream *sp, cJSON *json_interval_streams);
static cJSON *JSON_read(int fd);


/*************************** Print usage functions ****************************/

void
usage()
{
    fputs(usage_shortstr, stderr);
}


void
usage_long(FILE *f)
{
    fprintf(f, usage_longstr, UDP_RATE / (1024*1024), DURATION, DEFAULT_TCP_BLKSIZE / 1024, DEFAULT_UDP_BLKSIZE);
}


void warning(char *str)
{
    fprintf(stderr, "warning: %s\n", str);
}


/************** Getter routines for some fields inside iperf_test *************/

int
iperf_get_verbose(struct iperf_test *ipt)
{
    return ipt->verbose;
}

int
iperf_get_control_socket(struct iperf_test *ipt)
{
    return ipt->ctrl_sck;
}

int
iperf_get_control_socket_mss(struct iperf_test *ipt)
{
    return ipt->ctrl_sck_mss;
}

int
iperf_get_test_omit(struct iperf_test *ipt)
{
    return ipt->omit;
}

int
iperf_get_test_duration(struct iperf_test *ipt)
{
    return ipt->duration;
}

uint64_t
iperf_get_test_rate(struct iperf_test *ipt)
{
    return ipt->settings->rate;
}

uint64_t
iperf_get_test_fqrate(struct iperf_test *ipt)
{
    return ipt->settings->fqrate;
}

int
iperf_get_test_pacing_timer(struct iperf_test *ipt)
{
    return ipt->settings->pacing_timer;
}

uint64_t
iperf_get_test_bytes(struct iperf_test *ipt)
{
    return (uint64_t) ipt->settings->bytes;
}

uint64_t
iperf_get_test_blocks(struct iperf_test *ipt)
{
    return (uint64_t) ipt->settings->blocks;
}

int
iperf_get_test_burst(struct iperf_test *ipt)
{
    return ipt->settings->burst;
}

char
iperf_get_test_role(struct iperf_test *ipt)
{
    return ipt->role;
}

int
iperf_get_test_reverse(struct iperf_test *ipt)
{
    return ipt->reverse;
}

int
iperf_get_test_blksize(struct iperf_test *ipt)
{
    return ipt->settings->blksize;
}

FILE *
iperf_get_test_outfile (struct iperf_test *ipt)
{
    return ipt->outfile;
}

int
iperf_get_test_socket_bufsize(struct iperf_test *ipt)
{
    return ipt->settings->socket_bufsize;
}

double
iperf_get_test_reporter_interval(struct iperf_test *ipt)
{
    return ipt->reporter_interval;
}

double
iperf_get_test_stats_interval(struct iperf_test *ipt)
{
    return ipt->stats_interval;
}

int
iperf_get_test_num_streams(struct iperf_test *ipt)
{
    return ipt->num_streams;
}

int
iperf_get_test_repeating_payload(struct iperf_test *ipt)
{
    return ipt->repeating_payload;
}

int
iperf_get_test_server_port(struct iperf_test *ipt)
{
    return ipt->server_port;
}

char*
iperf_get_test_server_hostname(struct iperf_test *ipt)
{
    return ipt->server_hostname;
}

char*
iperf_get_test_template(struct iperf_test *ipt)
{
    return ipt->tmp_template;
}

int
iperf_get_test_protocol_id(struct iperf_test *ipt)
{
    return ipt->protocol->id;
}

int
iperf_get_test_json_output(struct iperf_test *ipt)
{
    return ipt->json_output;
}

char *
iperf_get_test_json_output_string(struct iperf_test *ipt)
{
    return ipt->json_output_string;
}

int
iperf_get_test_zerocopy(struct iperf_test *ipt)
{
    return ipt->zerocopy;
}

int
iperf_get_test_get_server_output(struct iperf_test *ipt)
{
    return ipt->get_server_output;
}

char
iperf_get_test_unit_format(struct iperf_test *ipt)
{
    return ipt->settings->unit_format;
}

char *
iperf_get_test_bind_address(struct iperf_test *ipt)
{
    return ipt->bind_address;
}

int
iperf_get_test_udp_counters_64bit(struct iperf_test *ipt)
{
    return ipt->udp_counters_64bit;
}

int
iperf_get_test_one_off(struct iperf_test *ipt)
{
    return ipt->one_off;
}

int
iperf_get_test_tos(struct iperf_test *ipt)
{
    return ipt->settings->tos;
}

char *
iperf_get_test_extra_data(struct iperf_test *ipt)
{
    return ipt->extra_data;
}

static const char iperf_version[] = IPERF_VERSION;
char *
iperf_get_iperf_version(void)
{
    return (char*)iperf_version;
}

int
iperf_get_test_no_delay(struct iperf_test *ipt)
{
    return ipt->no_delay;
}

int
iperf_get_test_connect_timeout(struct iperf_test *ipt)
{
    return ipt->settings->connect_timeout;
}

/************** Setter routines for some fields inside iperf_test *************/

void
iperf_set_verbose(struct iperf_test *ipt, int verbose)
{
    ipt->verbose = verbose;
}

void
iperf_set_control_socket(struct iperf_test *ipt, int ctrl_sck)
{
    ipt->ctrl_sck = ctrl_sck;
}

void
iperf_set_test_omit(struct iperf_test *ipt, int omit)
{
    ipt->omit = omit;
}

void
iperf_set_test_duration(struct iperf_test *ipt, int duration)
{
    ipt->duration = duration;
}

void
iperf_set_test_reporter_interval(struct iperf_test *ipt, double reporter_interval)
{
    ipt->reporter_interval = reporter_interval;
}

void
iperf_set_test_stats_interval(struct iperf_test *ipt, double stats_interval)
{
    ipt->stats_interval = stats_interval;
}

void
iperf_set_test_state(struct iperf_test *ipt, signed char state)
{
    ipt->state = state;
}

void
iperf_set_test_blksize(struct iperf_test *ipt, int blksize)
{
    ipt->settings->blksize = blksize;
}

void
iperf_set_test_logfile(struct iperf_test *ipt, char *logfile)
{
    ipt->logfile = strdup(logfile);
}

void
iperf_set_test_rate(struct iperf_test *ipt, uint64_t rate)
{
    ipt->settings->rate = rate;
}

void
iperf_set_test_fqrate(struct iperf_test *ipt, uint64_t fqrate)
{
    ipt->settings->fqrate = fqrate;
}

void
iperf_set_test_pacing_timer(struct iperf_test *ipt, int pacing_timer)
{
    ipt->settings->pacing_timer = pacing_timer;
}

void
iperf_set_test_bytes(struct iperf_test *ipt, uint64_t bytes)
{
    ipt->settings->bytes = (iperf_size_t) bytes;
}

void
iperf_set_test_blocks(struct iperf_test *ipt, uint64_t blocks)
{
    ipt->settings->blocks = (iperf_size_t) blocks;
}

void
iperf_set_test_burst(struct iperf_test *ipt, int burst)
{
    ipt->settings->burst = burst;
}

void
iperf_set_test_server_port(struct iperf_test *ipt, int srv_port)
{
    ipt->server_port = srv_port;
}

void
iperf_set_test_socket_bufsize(struct iperf_test *ipt, int socket_bufsize)
{
    ipt->settings->socket_bufsize = socket_bufsize;
}

void
iperf_set_test_num_streams(struct iperf_test *ipt, int num_streams)
{
    ipt->num_streams = num_streams;
}

void
iperf_set_test_repeating_payload(struct iperf_test *ipt, int repeating_payload)
{
    ipt->repeating_payload = repeating_payload;
}

static void
check_sender_has_retransmits(struct iperf_test *ipt)
{
    if (ipt->mode != RECEIVER && ipt->protocol->id == Ptcp && has_tcpinfo_retransmits())
	ipt->sender_has_retransmits = 1;
    else
	ipt->sender_has_retransmits = 0;
}

void
iperf_set_test_role(struct iperf_test *ipt, char role)
{
    ipt->role = role;
    if (!ipt->reverse) {
        if (ipt->bidirectional)
            ipt->mode = BIDIRECTIONAL;
        else if (role == 'c')
            ipt->mode = SENDER;
        else if (role == 's')
            ipt->mode = RECEIVER;
    } else {
        if (role == 'c')
            ipt->mode = RECEIVER;
        else if (role == 's')
            ipt->mode = SENDER;
    }
    check_sender_has_retransmits(ipt);
}

void
iperf_set_test_server_hostname(struct iperf_test *ipt, char *server_hostname)
{
    ipt->server_hostname = strdup(server_hostname);
}

void
iperf_set_test_template(struct iperf_test *ipt, char *tmp_template)
{
    ipt->tmp_template = strdup(tmp_template);
}

void
iperf_set_test_reverse(struct iperf_test *ipt, int reverse)
{
    ipt->reverse = reverse;
    if (!ipt->reverse) {
        if (ipt->role == 'c')
            ipt->mode = SENDER;
        else if (ipt->role == 's')
            ipt->mode = RECEIVER;
    } else {
        if (ipt->role == 'c')
            ipt->mode = RECEIVER;
        else if (ipt->role == 's')
            ipt->mode = SENDER;
    }
    check_sender_has_retransmits(ipt);
}

void
iperf_set_test_json_output(struct iperf_test *ipt, int json_output)
{
    ipt->json_output = json_output;
}

int
iperf_has_zerocopy( void )
{
    return has_sendfile();
}

void
iperf_set_test_zerocopy(struct iperf_test *ipt, int zerocopy)
{
    ipt->zerocopy = (zerocopy && has_sendfile());
}

void
iperf_set_test_get_server_output(struct iperf_test *ipt, int get_server_output)
{
    ipt->get_server_output = get_server_output;
}

void
iperf_set_test_unit_format(struct iperf_test *ipt, char unit_format)
{
    ipt->settings->unit_format = unit_format;
}

#if defined(HAVE_SSL)
void
iperf_set_test_client_username(struct iperf_test *ipt, char *client_username)
{
    ipt->settings->client_username = strdup(client_username);
}

void
iperf_set_test_client_password(struct iperf_test *ipt, char *client_password)
{
    ipt->settings->client_password = strdup(client_password);
}

void
iperf_set_test_client_rsa_pubkey(struct iperf_test *ipt, char *client_rsa_pubkey_base64)
{
    ipt->settings->client_rsa_pubkey = load_pubkey_from_base64(client_rsa_pubkey_base64);
}

void
iperf_set_test_server_authorized_users(struct iperf_test *ipt, char *server_authorized_users)
{
    ipt->server_authorized_users = strdup(server_authorized_users);
}

void
iperf_set_test_server_rsa_privkey(struct iperf_test *ipt, char *server_rsa_privkey_base64)
{
    ipt->server_rsa_private_key = load_privkey_from_base64(server_rsa_privkey_base64);
}
#endif // HAVE_SSL

void
iperf_set_test_bind_address(struct iperf_test *ipt, char *bnd_address)
{
    ipt->bind_address = strdup(bnd_address);
}

void
iperf_set_test_udp_counters_64bit(struct iperf_test *ipt, int udp_counters_64bit)
{
    ipt->udp_counters_64bit = udp_counters_64bit;
}

void
iperf_set_test_one_off(struct iperf_test *ipt, int one_off)
{
    ipt->one_off = one_off;
}

void
iperf_set_test_tos(struct iperf_test *ipt, int tos)
{
    ipt->settings->tos = tos;
}

void
iperf_set_test_extra_data(struct iperf_test *ipt, char *dat)
{
    ipt->extra_data = strdup(dat);
}

void
iperf_set_test_bidirectional(struct iperf_test* ipt, int bidirectional)
{
    ipt->bidirectional = bidirectional;
    if (bidirectional)
        ipt->mode = BIDIRECTIONAL;
    else
        iperf_set_test_reverse(ipt, ipt->reverse);
}

void
iperf_set_test_no_delay(struct iperf_test* ipt, int no_delay)
{
    ipt->no_delay = no_delay;
}

void
iperf_set_test_connect_timeout(struct iperf_test* ipt, int ct)
{
    ipt->settings->connect_timeout = ct;
}


/********************** Get/set test protocol structure ***********************/

struct protocol *
get_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id)
            break;
    }

    if (prot == NULL)
        i_errno = IEPROTOCOL;

    return prot;
}

int
set_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot = NULL;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id) {
            test->protocol = prot;
	    check_sender_has_retransmits(test);
            return 0;
        }
    }

    i_errno = IEPROTOCOL;
    return -1;
}


/************************** Iperf callback functions **************************/

void
iperf_on_new_stream(struct iperf_stream *sp)
{
    connect_msg(sp);
}

void
iperf_on_test_start(struct iperf_test *test)
{
    if (test->json_output) {
	cJSON_AddItemToObject(test->json_start, "test_start", iperf_json_printf("protocol: %s  num_streams: %d  blksize: %d  omit: %d  duration: %d  bytes: %d  blocks: %d  reverse: %d  tos: %d", test->protocol->name, (int64_t) test->num_streams, (int64_t) test->settings->blksize, (int64_t) test->omit, (int64_t) test->duration, (int64_t) test->settings->bytes, (int64_t) test->settings->blocks, test->reverse?(int64_t)1:(int64_t)0, (int64_t) test->settings->tos));
    } else {
	if (test->verbose) {
	    if (test->settings->bytes)
		iperf_printf(test, test_start_bytes, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->settings->bytes, test->settings->tos);
	    else if (test->settings->blocks)
		iperf_printf(test, test_start_blocks, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->settings->blocks, test->settings->tos);
	    else
		iperf_printf(test, test_start_time, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->duration, test->settings->tos);
	}
    }
}

/* This converts an IPv6 string address from IPv4-mapped format into regular
** old IPv4 format, which is easier on the eyes of network veterans.
**
** If the v6 address is not v4-mapped it is left alone.
*/
static void
mapped_v4_to_regular_v4(char *str)
{
    char *prefix = "::ffff:";
    int prefix_len;

    prefix_len = strlen(prefix);
    if (strncmp(str, prefix, prefix_len) == 0) {
	int str_len = strlen(str);
	memmove(str, str + prefix_len, str_len - prefix_len + 1);
    }
}

void
iperf_on_connect(struct iperf_test *test)
{
    time_t now_secs;
    const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S %Z";
    char now_str[100];
    char ipr[INET6_ADDRSTRLEN];
    int port;
    struct sockaddr_storage sa;
    struct sockaddr_in *sa_inP;
    struct sockaddr_in6 *sa_in6P;
    socklen_t len;

    now_secs = time((time_t*) 0);
    (void) strftime(now_str, sizeof(now_str), rfc1123_fmt, gmtime(&now_secs));
    if (test->json_output)
	cJSON_AddItemToObject(test->json_start, "timestamp", iperf_json_printf("time: %s  timesecs: %d", now_str, (int64_t) now_secs));
    else if (test->verbose)
	iperf_printf(test, report_time, now_str);

    if (test->role == 'c') {
	if (test->json_output)
	    cJSON_AddItemToObject(test->json_start, "connecting_to", iperf_json_printf("host: %s  port: %d", test->server_hostname, (int64_t) test->server_port));
	else {
	    iperf_printf(test, report_connecting, test->server_hostname, test->server_port);
	    if (test->reverse)
		iperf_printf(test, report_reverse, test->server_hostname);
	}
    } else {
        len = sizeof(sa);
        getpeername(test->ctrl_sck, (struct sockaddr *) &sa, &len);
        if (getsockdomain(test->ctrl_sck) == AF_INET) {
	    sa_inP = (struct sockaddr_in *) &sa;
            inet_ntop(AF_INET, &sa_inP->sin_addr, ipr, sizeof(ipr));
	    port = ntohs(sa_inP->sin_port);
        } else {
	    sa_in6P = (struct sockaddr_in6 *) &sa;
            inet_ntop(AF_INET6, &sa_in6P->sin6_addr, ipr, sizeof(ipr));
	    port = ntohs(sa_in6P->sin6_port);
        }
	mapped_v4_to_regular_v4(ipr);
	if (test->json_output)
	    cJSON_AddItemToObject(test->json_start, "accepted_connection", iperf_json_printf("host: %s  port: %d", ipr, (int64_t) port));
	else
	    iperf_printf(test, report_accepted, ipr, port);
    }
    if (test->json_output) {
	cJSON_AddStringToObject(test->json_start, "cookie", test->cookie);
        if (test->protocol->id == SOCK_STREAM) {
	    if (test->settings->mss)
		cJSON_AddNumberToObject(test->json_start, "tcp_mss", test->settings->mss);
	    else {
		cJSON_AddNumberToObject(test->json_start, "tcp_mss_default", test->ctrl_sck_mss);
	    }
        if (test->settings->rate)
            cJSON_AddNumberToObject(test->json_start, "target_bitrate", test->settings->rate);
        }
    } else if (test->verbose) {
        iperf_printf(test, report_cookie, test->cookie);
        if (test->protocol->id == SOCK_STREAM) {
            if (test->settings->mss)
                iperf_printf(test, "      TCP MSS: %d\n", test->settings->mss);
            else {
                iperf_printf(test, "      TCP MSS: %d (default)\n", test->ctrl_sck_mss);
            }
        }
        if (test->settings->rate)
            iperf_printf(test, "      Target Bitrate: %"PRIu64"\n", test->settings->rate);
    }
}

void
iperf_on_test_finish(struct iperf_test *test)
{
}


/******************************************************************************/

int
iperf_parse_arguments(struct iperf_test *test, int argc, char **argv)
{
    static struct option longopts[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"format", required_argument, NULL, 'f'},
        {"interval", required_argument, NULL, 'i'},
        {"daemon", no_argument, NULL, 'D'},
        {"one-off", no_argument, NULL, '1'},
        {"verbose", no_argument, NULL, 'V'},
        {"json", no_argument, NULL, 'J'},
        {"version", no_argument, NULL, 'v'},
        {"server", no_argument, NULL, 's'},
        {"client", required_argument, NULL, 'c'},
        {"udp", no_argument, NULL, 'u'},
        {"bitrate", required_argument, NULL, 'b'},
        {"bandwidth", required_argument, NULL, 'b'},
        {"time", required_argument, NULL, 't'},
        {"bytes", required_argument, NULL, 'n'},
        {"blockcount", required_argument, NULL, 'k'},
        {"length", required_argument, NULL, 'l'},
        {"parallel", required_argument, NULL, 'P'},
        {"reverse", no_argument, NULL, 'R'},
        {"bidir", no_argument, NULL, OPT_BIDIRECTIONAL},
        {"window", required_argument, NULL, 'w'},
        {"bind", required_argument, NULL, 'B'},
        {"cport", required_argument, NULL, OPT_CLIENT_PORT},
        {"set-mss", required_argument, NULL, 'M'},
        {"no-delay", no_argument, NULL, 'N'},
        {"version4", no_argument, NULL, '4'},
        {"version6", no_argument, NULL, '6'},
        {"tos", required_argument, NULL, 'S'},
        {"dscp", required_argument, NULL, OPT_DSCP},
	{"extra-data", required_argument, NULL, OPT_EXTRA_DATA},
#if defined(HAVE_FLOWLABEL)
        {"flowlabel", required_argument, NULL, 'L'},
#endif /* HAVE_FLOWLABEL */
        {"zerocopy", no_argument, NULL, 'Z'},
        {"omit", required_argument, NULL, 'O'},
        {"file", required_argument, NULL, 'F'},
        {"repeating-payload", no_argument, NULL, OPT_REPEATING_PAYLOAD},
#if defined(HAVE_CPU_AFFINITY)
        {"affinity", required_argument, NULL, 'A'},
#endif /* HAVE_CPU_AFFINITY */
        {"title", required_argument, NULL, 'T'},
#if defined(HAVE_TCP_CONGESTION)
        {"congestion", required_argument, NULL, 'C'},
        {"linux-congestion", required_argument, NULL, 'C'},
#endif /* HAVE_TCP_CONGESTION */
#if defined(HAVE_SCTP)
        {"sctp", no_argument, NULL, OPT_SCTP},
        {"nstreams", required_argument, NULL, OPT_NUMSTREAMS},
        {"xbind", required_argument, NULL, 'X'},
#endif
	{"pidfile", required_argument, NULL, 'I'},
	{"logfile", required_argument, NULL, OPT_LOGFILE},
	{"forceflush", no_argument, NULL, OPT_FORCEFLUSH},
	{"get-server-output", no_argument, NULL, OPT_GET_SERVER_OUTPUT},
	{"udp-counters-64bit", no_argument, NULL, OPT_UDP_COUNTERS_64BIT},
 	{"no-fq-socket-pacing", no_argument, NULL, OPT_NO_FQ_SOCKET_PACING},
#if defined(HAVE_SSL)
    {"username", required_argument, NULL, OPT_CLIENT_USERNAME},
    {"rsa-public-key-path", required_argument, NULL, OPT_CLIENT_RSA_PUBLIC_KEY},
    {"rsa-private-key-path", required_argument, NULL, OPT_SERVER_RSA_PRIVATE_KEY},
    {"authorized-users-path", required_argument, NULL, OPT_SERVER_AUTHORIZED_USERS},
#endif /* HAVE_SSL */
	{"fq-rate", required_argument, NULL, OPT_FQ_RATE},
	{"pacing-timer", required_argument, NULL, OPT_PACING_TIMER},
	{"connect-timeout", required_argument, NULL, OPT_CONNECT_TIMEOUT},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };
    int flag;
    int portno;
    int blksize;
    int server_flag, client_flag, rate_flag, duration_flag;
    char *endptr;
#if defined(HAVE_CPU_AFFINITY)
    char* comma;
#endif /* HAVE_CPU_AFFINITY */
    char* slash;
    struct xbind_entry *xbe;
    double farg;

    blksize = 0;
    server_flag = client_flag = rate_flag = duration_flag = 0;
#if defined(HAVE_SSL)
    char *client_username = NULL, *client_rsa_public_key = NULL, *server_rsa_private_key = NULL;
#endif /* HAVE_SSL */

    while ((flag = getopt_long(argc, argv, "p:f:i:D1VJvsc:ub:t:n:k:l:P:Rw:B:M:N46S:L:ZO:F:A:T:C:dI:hX:", longopts, NULL)) != -1) {
        switch (flag) {
            case 'p':
		portno = atoi(optarg);
		if (portno < 1 || portno > 65535) {
		    i_errno = IEBADPORT;
		    return -1;
		}
		test->server_port = portno;
                break;
            case 'f':
		if (!optarg) {
		    i_errno = IEBADFORMAT;
		    return -1;
		}
		test->settings->unit_format = *optarg;
		if (test->settings->unit_format == 'k' ||
		    test->settings->unit_format == 'K' ||
		    test->settings->unit_format == 'm' ||
		    test->settings->unit_format == 'M' ||
		    test->settings->unit_format == 'g' ||
		    test->settings->unit_format == 'G' ||
		    test->settings->unit_format == 't' ||
		    test->settings->unit_format == 'T') {
			break;
		}
		else {
		    i_errno = IEBADFORMAT;
		    return -1;
		}
                break;
            case 'i':
                /* XXX: could potentially want separate stat collection and reporting intervals,
                   but just set them to be the same for now */
                test->stats_interval = test->reporter_interval = atof(optarg);
                if ((test->stats_interval < MIN_INTERVAL || test->stats_interval > MAX_INTERVAL) && test->stats_interval != 0) {
                    i_errno = IEINTERVAL;
                    return -1;
                }
                break;
            case 'D':
		test->daemon = 1;
		server_flag = 1;
	        break;
            case '1':
		test->one_off = 1;
		server_flag = 1;
	        break;
            case 'V':
                test->verbose = 1;
                break;
            case 'J':
                test->json_output = 1;
                break;
            case 'v':
                printf("%s (cJSON %s)\n%s\n%s\n", version, cJSON_Version(), get_system_info(),
		       get_optional_features());
                exit(0);
            case 's':
                if (test->role == 'c') {
                    i_errno = IESERVCLIENT;
                    return -1;
                }
		iperf_set_test_role(test, 's');
                break;
            case 'c':
                if (test->role == 's') {
                    i_errno = IESERVCLIENT;
                    return -1;
                }
		iperf_set_test_role(test, 'c');
		iperf_set_test_server_hostname(test, optarg);
                break;
            case 'u':
                set_protocol(test, Pudp);
		client_flag = 1;
                break;
            case OPT_SCTP:
#if defined(HAVE_SCTP)
                set_protocol(test, Psctp);
                client_flag = 1;
                break;
#else /* HAVE_SCTP */
                i_errno = IEUNIMP;
                return -1;
#endif /* HAVE_SCTP */

            case OPT_NUMSTREAMS:
#if defined(linux) || defined(__FreeBSD__)
                test->settings->num_ostreams = unit_atoi(optarg);
                client_flag = 1;
#else /* linux */
                i_errno = IEUNIMP;
                return -1;
#endif /* linux */
            case 'b':
		slash = strchr(optarg, '/');
		if (slash) {
		    *slash = '\0';
		    ++slash;
		    test->settings->burst = atoi(slash);
		    if (test->settings->burst <= 0 ||
		        test->settings->burst > MAX_BURST) {
			i_errno = IEBURST;
			return -1;
		    }
		}
                test->settings->rate = unit_atof_rate(optarg);
		rate_flag = 1;
		client_flag = 1;
                break;
            case 't':
                test->duration = atoi(optarg);
                if (test->duration > MAX_TIME) {
                    i_errno = IEDURATION;
                    return -1;
                }
		duration_flag = 1;
		client_flag = 1;
                break;
            case 'n':
                test->settings->bytes = unit_atoi(optarg);
		client_flag = 1;
                break;
            case 'k':
                test->settings->blocks = unit_atoi(optarg);
		client_flag = 1;
                break;
            case 'l':
                blksize = unit_atoi(optarg);
		client_flag = 1;
                break;
            case 'P':
                test->num_streams = atoi(optarg);
                if (test->num_streams > MAX_STREAMS) {
                    i_errno = IENUMSTREAMS;
                    return -1;
                }
		client_flag = 1;
                break;
            case 'R':
                if (test->bidirectional) {
                    i_errno = IEREVERSEBIDIR;
                    return -1;
                }
		iperf_set_test_reverse(test, 1);
		client_flag = 1;
                break;
            case OPT_BIDIRECTIONAL:
                if (test->reverse) {
                    i_errno = IEREVERSEBIDIR;
                    return -1;
                }
                iperf_set_test_bidirectional(test, 1);
                client_flag = 1;
                break;
            case 'w':
                // XXX: This is a socket buffer, not specific to TCP
		// Do sanity checks as double-precision floating point 
		// to avoid possible integer overflows.
                farg = unit_atof(optarg);
                if (farg > (double) MAX_TCP_BUFFER) {
                    i_errno = IEBUFSIZE;
                    return -1;
                }
                test->settings->socket_bufsize = (int) farg;
		client_flag = 1;
                break;
            case 'B':
                test->bind_address = strdup(optarg);
                break;
            case OPT_CLIENT_PORT:
		portno = atoi(optarg);
		if (portno < 1 || portno > 65535) {
		    i_errno = IEBADPORT;
		    return -1;
		}
                test->bind_port = portno;
                break;
            case 'M':
                test->settings->mss = atoi(optarg);
                if (test->settings->mss > MAX_MSS) {
                    i_errno = IEMSS;
                    return -1;
                }
		client_flag = 1;
                break;
            case 'N':
                test->no_delay = 1;
		client_flag = 1;
                break;
            case '4':
                test->settings->domain = AF_INET;
                break;
            case '6':
                test->settings->domain = AF_INET6;
                break;
            case 'S':
                test->settings->tos = strtol(optarg, &endptr, 0);
		if (endptr == optarg ||
		    test->settings->tos < 0 ||
		    test->settings->tos > 255) {
		    i_errno = IEBADTOS;
		    return -1;
		}
		client_flag = 1;
                break;
	    case OPT_DSCP:
                test->settings->tos = parse_qos(optarg);
		if(test->settings->tos < 0) {
			i_errno = IEBADTOS;
			return -1;
		}
		client_flag = 1;
                break;
	    case OPT_EXTRA_DATA:
		test->extra_data = strdup(optarg);
		client_flag = 1;
	        break;
            case 'L':
#if defined(HAVE_FLOWLABEL)
                test->settings->flowlabel = strtol(optarg, &endptr, 0);
		if (endptr == optarg ||
		    test->settings->flowlabel < 1 || test->settings->flowlabel > 0xfffff) {
                    i_errno = IESETFLOW;
                    return -1;
		}
		client_flag = 1;
#else /* HAVE_FLOWLABEL */
                i_errno = IEUNIMP;
                return -1;
#endif /* HAVE_FLOWLABEL */
                break;
            case 'X':
		xbe = (struct xbind_entry *)malloc(sizeof(struct xbind_entry));
                if (!xbe) {
		    i_errno = IESETSCTPBINDX;
                    return -1;
                }
	        memset(xbe, 0, sizeof(*xbe));
                xbe->name = strdup(optarg);
                if (!xbe->name) {
		    i_errno = IESETSCTPBINDX;
                    return -1;
                }
		TAILQ_INSERT_TAIL(&test->xbind_addrs, xbe, link);
                break;
            case 'Z':
                if (!has_sendfile()) {
                    i_errno = IENOSENDFILE;
                    return -1;
                }
                test->zerocopy = 1;
		client_flag = 1;
                break;
            case OPT_REPEATING_PAYLOAD:
                test->repeating_payload = 1;
                client_flag = 1;
                break;
            case 'O':
                test->omit = atoi(optarg);
                if (test->omit < 0 || test->omit > 60) {
                    i_errno = IEOMIT;
                    return -1;
                }
		client_flag = 1;
                break;
            case 'F':
                test->diskfile_name = optarg;
                break;
            case 'A':
#if defined(HAVE_CPU_AFFINITY)
                test->affinity = strtol(optarg, &endptr, 0);
                if (endptr == optarg || 
		    test->affinity < 0 || test->affinity > 1024) {
                    i_errno = IEAFFINITY;
                    return -1;
                }
		comma = strchr(optarg, ',');
		if (comma != NULL) {
		    test->server_affinity = atoi(comma+1);
		    if (test->server_affinity < 0 || test->server_affinity > 1024) {
			i_errno = IEAFFINITY;
			return -1;
		    }
		    client_flag = 1;
		}
#else /* HAVE_CPU_AFFINITY */
                i_errno = IEUNIMP;
                return -1;
#endif /* HAVE_CPU_AFFINITY */
                break;
            case 'T':
                test->title = strdup(optarg);
		client_flag = 1;
                break;
	    case 'C':
#if defined(HAVE_TCP_CONGESTION)
		test->congestion = strdup(optarg);
		client_flag = 1;
#else /* HAVE_TCP_CONGESTION */
		i_errno = IEUNIMP;
		return -1;
#endif /* HAVE_TCP_CONGESTION */
		break;
	    case 'd':
		test->debug = 1;
		break;
	    case 'I':
		test->pidfile = strdup(optarg);
		server_flag = 1;
	        break;
	    case OPT_LOGFILE:
		test->logfile = strdup(optarg);
		break;
	    case OPT_FORCEFLUSH:
		test->forceflush = 1;
		break;
	    case OPT_GET_SERVER_OUTPUT:
		test->get_server_output = 1;
		client_flag = 1;
		break;
	    case OPT_UDP_COUNTERS_64BIT:
		test->udp_counters_64bit = 1;
		break;
	    case OPT_NO_FQ_SOCKET_PACING:
#if defined(HAVE_SO_MAX_PACING_RATE)
		printf("Warning:  --no-fq-socket-pacing is deprecated\n");
		test->settings->fqrate = 0;
		client_flag = 1;
#else /* HAVE_SO_MAX_PACING_RATE */
		i_errno = IEUNIMP;
		return -1;
#endif
		break;
	    case OPT_FQ_RATE:
#if defined(HAVE_SO_MAX_PACING_RATE)
		test->settings->fqrate = unit_atof_rate(optarg);
		client_flag = 1;
#else /* HAVE_SO_MAX_PACING_RATE */
		i_errno = IEUNIMP;
		return -1;
#endif
		break;
#if defined(HAVE_SSL)
        case OPT_CLIENT_USERNAME:
            client_username = strdup(optarg);
            break;
        case OPT_CLIENT_RSA_PUBLIC_KEY:
            client_rsa_public_key = strdup(optarg);
            break;
        case OPT_SERVER_RSA_PRIVATE_KEY:
            server_rsa_private_key = strdup(optarg);
            break;
        case OPT_SERVER_AUTHORIZED_USERS:
            test->server_authorized_users = strdup(optarg);
            break;
#endif /* HAVE_SSL */
	    case OPT_PACING_TIMER:
		test->settings->pacing_timer = unit_atoi(optarg);
		client_flag = 1;
		break;
	    case OPT_CONNECT_TIMEOUT:
		test->settings->connect_timeout = unit_atoi(optarg);
		client_flag = 1;
		break;
	    case 'h':
		usage_long(stdout);
		exit(0);
            default:
                usage_long(stderr);
                exit(1);
        }
    }

    /* Check flag / role compatibility. */
    if (test->role == 'c' && server_flag) {
        i_errno = IESERVERONLY;
        return -1;
    }
    if (test->role == 's' && client_flag) {
        i_errno = IECLIENTONLY;
        return -1;
    }

#if defined(HAVE_SSL)

    if (test->role == 's' && (client_username || client_rsa_public_key)){
        i_errno = IECLIENTONLY;
        return -1;
    } else if (test->role == 'c' && (client_username || client_rsa_public_key) && 
        !(client_username && client_rsa_public_key)) {
        i_errno = IESETCLIENTAUTH;
        return -1;
    } else if (test->role == 'c' && (client_username && client_rsa_public_key)){

        char *client_password = NULL;
        size_t s;
        /* Need to copy env var, so we can do a common free */
        if ((client_password = getenv("IPERF3_PASSWORD")) != NULL)
             client_password = strdup(client_password);
        else if (iperf_getpass(&client_password, &s, stdin) < 0){
            return -1;
        } 
        if (test_load_pubkey_from_file(client_rsa_public_key) < 0){
            i_errno = IESETCLIENTAUTH;
            return -1;
        }

        test->settings->client_username = client_username;
        test->settings->client_password = client_password;
        test->settings->client_rsa_pubkey = load_pubkey_from_file(client_rsa_public_key);
	free(client_rsa_public_key);
	client_rsa_public_key = NULL;
    }

    if (test->role == 'c' && (server_rsa_private_key || test->server_authorized_users)){
        i_errno = IESERVERONLY;
        return -1;
    } else if (test->role == 's' && (server_rsa_private_key || test->server_authorized_users) && 
        !(server_rsa_private_key && test->server_authorized_users)) {
         i_errno = IESETSERVERAUTH;
        return -1;
    } else if (test->role == 's' && server_rsa_private_key) {
        test->server_rsa_private_key = load_privkey_from_file(server_rsa_private_key);
        if (test->server_rsa_private_key == NULL){
            i_errno = IESETSERVERAUTH;
            return -1;
        }
	free(server_rsa_private_key);
	server_rsa_private_key = NULL;
    }

#endif //HAVE_SSL
    if (blksize == 0) {
	if (test->protocol->id == Pudp)
	    blksize = 0;	/* try to dynamically determine from MSS */
	else if (test->protocol->id == Psctp)
	    blksize = DEFAULT_SCTP_BLKSIZE;
	else
	    blksize = DEFAULT_TCP_BLKSIZE;
    }
    if ((test->protocol->id != Pudp && blksize <= 0) 
	|| blksize > MAX_BLOCKSIZE) {
	i_errno = IEBLOCKSIZE;
	return -1;
    }
    if (test->protocol->id == Pudp &&
	(blksize > 0 &&
	    (blksize < MIN_UDP_BLOCKSIZE || blksize > MAX_UDP_BLOCKSIZE))) {
	i_errno = IEUDPBLOCKSIZE;
	return -1;
    }
    test->settings->blksize = blksize;

    if (!rate_flag)
	test->settings->rate = test->protocol->id == Pudp ? UDP_RATE : 0;

    if ((test->settings->bytes != 0 || test->settings->blocks != 0) && ! duration_flag)
        test->duration = 0;

    /* Disallow specifying multiple test end conditions. The code actually
    ** works just fine without this prohibition. As soon as any one of the
    ** three possible end conditions is met, the test ends. So this check
    ** could be removed if desired.
    */
    if ((duration_flag && test->settings->bytes != 0) ||
        (duration_flag && test->settings->blocks != 0) ||
	(test->settings->bytes != 0 && test->settings->blocks != 0)) {
        i_errno = IEENDCONDITIONS;
        return -1;
    }

    /* For subsequent calls to getopt */
#ifdef __APPLE__
    optreset = 1;
#endif
    optind = 0;

    if ((test->role != 'c') && (test->role != 's')) {
        i_errno = IENOROLE;
        return -1;
    }

    /* Show warning if JSON output is used with explicit report format */
    if ((test->json_output) && (test->settings->unit_format != 'a')) {
        warning("Report format (-f) flag ignored with JSON output (-J)");
    }

    /* Show warning if JSON output is used with verbose or debug flags */
    if (test->json_output && test->verbose) {
        warning("Verbose output (-v) may interfere with JSON output (-J)");
    }
    if (test->json_output && test->debug) {
        warning("Debug output (-d) may interfere with JSON output (-J)");
    }

    return 0;
}

/*
 * Open the file specified by test->logfile and set test->outfile to its' FD.
 */
int iperf_open_logfile(struct iperf_test *test)
{
    test->outfile = fopen(test->logfile, "a+");
    if (test->outfile == NULL) {
        i_errno = IELOGFILE;
        return -1;
    }

    return 0;
}

int
iperf_set_send_state(struct iperf_test *test, signed char state)
{
    test->state = state;
    if (Nwrite(test->ctrl_sck, (char*) &state, sizeof(state), Ptcp) < 0) {
	i_errno = IESENDMESSAGE;
	return -1;
    }
    return 0;
}

void
iperf_check_throttle(struct iperf_stream *sp, struct iperf_time *nowP)
{
    struct iperf_time temp_time;
    double seconds;
    uint64_t bits_per_second;

    if (sp->test->done || sp->test->settings->rate == 0 || sp->test->settings->burst != 0)
        return;
    iperf_time_diff(&sp->result->start_time_fixed, nowP, &temp_time);
    seconds = iperf_time_in_secs(&temp_time);
    bits_per_second = sp->result->bytes_sent * 8 / seconds;
    if (bits_per_second < sp->test->settings->rate) {
        sp->green_light = 1;
        FD_SET(sp->socket, &sp->test->write_set);
    } else {
        sp->green_light = 0;
        FD_CLR(sp->socket, &sp->test->write_set);
    }
}

int
iperf_send(struct iperf_test *test, fd_set *write_setP)
{
    register int multisend, r, streams_active;
    register struct iperf_stream *sp;
    struct iperf_time now;

    /* Can we do multisend mode? */
    if (test->settings->burst != 0)
        multisend = test->settings->burst;
    else if (test->settings->rate == 0)
        multisend = test->multisend;
    else
        multisend = 1;	/* nope */

    for (; multisend > 0; --multisend) {
	if (test->settings->rate != 0 && test->settings->burst == 0)
	    iperf_time_now(&now);
	streams_active = 0;
	SLIST_FOREACH(sp, &test->streams, streams) {
	    if ((sp->green_light && sp->sender &&
		 (write_setP == NULL || FD_ISSET(sp->socket, write_setP)))) {
		if ((r = sp->snd(sp)) < 0) {
		    if (r == NET_SOFTERROR)
			break;
		    i_errno = IESTREAMWRITE;
		    return r;
		}
		streams_active = 1;
		test->bytes_sent += r;
		++test->blocks_sent;
		iperf_check_throttle(sp, &now);
		if (multisend > 1 && test->settings->bytes != 0 && test->bytes_sent >= test->settings->bytes)
		    break;
		if (multisend > 1 && test->settings->blocks != 0 && test->blocks_sent >= test->settings->blocks)
		    break;
	    }
	}
	if (!streams_active)
	    break;
    }
    if (test->settings->burst != 0) {
	iperf_time_now(&now);
	SLIST_FOREACH(sp, &test->streams, streams)
	    if (sp->sender)
	        iperf_check_throttle(sp, &now);
    }
    if (write_setP != NULL)
	SLIST_FOREACH(sp, &test->streams, streams)
	    if (FD_ISSET(sp->socket, write_setP))
		FD_CLR(sp->socket, write_setP);

    return 0;
}

int
iperf_recv(struct iperf_test *test, fd_set *read_setP)
{
    int r;
    struct iperf_stream *sp;

    SLIST_FOREACH(sp, &test->streams, streams) {
	if (FD_ISSET(sp->socket, read_setP) && !sp->sender) {
	    if ((r = sp->rcv(sp)) < 0) {
		i_errno = IESTREAMREAD;
		return r;
	    }
	    test->bytes_received += r;
	    ++test->blocks_received;
	    FD_CLR(sp->socket, read_setP);
	}
    }

    return 0;
}

int
iperf_init_test(struct iperf_test *test)
{
    struct iperf_time now;
    struct iperf_stream *sp;

    if (test->protocol->init) {
        if (test->protocol->init(test) < 0)
            return -1;
    }

    /* Init each stream. */
    if (iperf_time_now(&now) < 0) {
	i_errno = IEINITTEST;
	return -1;
    }
    SLIST_FOREACH(sp, &test->streams, streams) {
	sp->result->start_time = sp->result->start_time_fixed = now;
    }

    if (test->on_test_start)
        test->on_test_start(test);

    return 0;
}

static void
send_timer_proc(TimerClientData client_data, struct iperf_time *nowP)
{
    struct iperf_stream *sp = client_data.p;

    /* All we do here is set or clear the flag saying that this stream may
    ** be sent to.  The actual sending gets done in the send proc, after
    ** checking the flag.
    */
    iperf_check_throttle(sp, nowP);
}

int
iperf_create_send_timers(struct iperf_test * test)
{
    struct iperf_time now;
    struct iperf_stream *sp;
    TimerClientData cd;

    if (iperf_time_now(&now) < 0) {
	i_errno = IEINITTEST;
	return -1;
    }
    SLIST_FOREACH(sp, &test->streams, streams) {
        sp->green_light = 1;
	if (test->settings->rate != 0 && sp->sender) {
	    cd.p = sp;
	    sp->send_timer = tmr_create(NULL, send_timer_proc, cd, test->settings->pacing_timer, 1);
	    if (sp->send_timer == NULL) {
		i_errno = IEINITTEST;
		return -1;
	    }
	}
    }
    return 0;
}

#if defined(HAVE_SSL)
int test_is_authorized(struct iperf_test *test){
    if ( !(test->server_rsa_private_key && test->server_authorized_users)) {
        return 0;
    }

    if (test->settings->authtoken){
        char *username = NULL, *password = NULL;
        time_t ts;
        int rc = decode_auth_setting(test->debug, test->settings->authtoken, test->server_rsa_private_key, &username, &password, &ts);
	if (rc) {
	    return -1;
	}
        int ret = check_authentication(username, password, ts, test->server_authorized_users);
        if (ret == 0){
            iperf_printf(test, report_authentication_succeeded, username, ts);
            free(username);
            free(password);
            return 0;
        } else {
            iperf_printf(test, report_authentication_failed, username, ts);
            free(username);
            free(password);
            return -1;
        }
    }
    return -1;
}
#endif //HAVE_SSL

/**
 * iperf_exchange_parameters - handles the param_Exchange part for client
 *
 */

int
iperf_exchange_parameters(struct iperf_test *test)
{
    int s;
    int32_t err;

    if (test->role == 'c') {

        if (send_parameters(test) < 0)
            return -1;

    } else {

        if (get_parameters(test) < 0)
            return -1;

#if defined(HAVE_SSL)
        if (test_is_authorized(test) < 0){
            if (iperf_set_send_state(test, SERVER_ERROR) != 0)
                return -1;
            i_errno = IEAUTHTEST;
            err = htonl(i_errno);
            if (Nwrite(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return -1;
            }
            return -1;
        }
#endif //HAVE_SSL

        if ((s = test->protocol->listen(test)) < 0) {
	        if (iperf_set_send_state(test, SERVER_ERROR) != 0)
                return -1;
            err = htonl(i_errno);
            if (Nwrite(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return -1;
            }
            err = htonl(errno);
            if (Nwrite(test->ctrl_sck, (char*) &err, sizeof(err), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return -1;
            }
            return -1;
        }
        FD_SET(s, &test->read_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->prot_listener = s;

        // Send the control message to create streams and start the test
	if (iperf_set_send_state(test, CREATE_STREAMS) != 0)
            return -1;

    }

    return 0;
}

/*************************************************************/

int
iperf_exchange_results(struct iperf_test *test)
{
    if (test->role == 'c') {
        /* Send results to server. */
	if (send_results(test) < 0)
            return -1;
        /* Get server results. */
        if (get_results(test) < 0)
            return -1;
    } else {
        /* Get client results. */
        if (get_results(test) < 0)
            return -1;
        /* Send results to client. */
	if (send_results(test) < 0)
            return -1;
    }
    return 0;
}

/*************************************************************/

static int
send_parameters(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;

    j = cJSON_CreateObject();
    if (j == NULL) {
	i_errno = IESENDPARAMS;
	r = -1;
    } else {
	if (test->protocol->id == Ptcp)
	    cJSON_AddTrueToObject(j, "tcp");
	else if (test->protocol->id == Pudp)
	    cJSON_AddTrueToObject(j, "udp");
        else if (test->protocol->id == Psctp)
            cJSON_AddTrueToObject(j, "sctp");
	cJSON_AddNumberToObject(j, "omit", test->omit);
	if (test->server_affinity != -1)
	    cJSON_AddNumberToObject(j, "server_affinity", test->server_affinity);
	cJSON_AddNumberToObject(j, "time", test->duration);
	if (test->settings->bytes)
	    cJSON_AddNumberToObject(j, "num", test->settings->bytes);
	if (test->settings->blocks)
	    cJSON_AddNumberToObject(j, "blockcount", test->settings->blocks);
	if (test->settings->mss)
	    cJSON_AddNumberToObject(j, "MSS", test->settings->mss);
	if (test->no_delay)
	    cJSON_AddTrueToObject(j, "nodelay");
	cJSON_AddNumberToObject(j, "parallel", test->num_streams);
	if (test->reverse)
	    cJSON_AddTrueToObject(j, "reverse");
	if (test->bidirectional)
	            cJSON_AddTrueToObject(j, "bidirectional");
	if (test->settings->socket_bufsize)
	    cJSON_AddNumberToObject(j, "window", test->settings->socket_bufsize);
	if (test->settings->blksize)
	    cJSON_AddNumberToObject(j, "len", test->settings->blksize);
	if (test->settings->rate)
	    cJSON_AddNumberToObject(j, "bandwidth", test->settings->rate);
	if (test->settings->fqrate)
	    cJSON_AddNumberToObject(j, "fqrate", test->settings->fqrate);
	if (test->settings->pacing_timer)
	    cJSON_AddNumberToObject(j, "pacing_timer", test->settings->pacing_timer);
	if (test->settings->burst)
	    cJSON_AddNumberToObject(j, "burst", test->settings->burst);
	if (test->settings->tos)
	    cJSON_AddNumberToObject(j, "TOS", test->settings->tos);
	if (test->settings->flowlabel)
	    cJSON_AddNumberToObject(j, "flowlabel", test->settings->flowlabel);
	if (test->title)
	    cJSON_AddStringToObject(j, "title", test->title);
	if (test->extra_data)
	    cJSON_AddStringToObject(j, "extra_data", test->extra_data);
	if (test->congestion)
	    cJSON_AddStringToObject(j, "congestion", test->congestion);
	if (test->congestion_used)
	    cJSON_AddStringToObject(j, "congestion_used", test->congestion_used);
	if (test->get_server_output)
	    cJSON_AddNumberToObject(j, "get_server_output", iperf_get_test_get_server_output(test));
	if (test->udp_counters_64bit)
	    cJSON_AddNumberToObject(j, "udp_counters_64bit", iperf_get_test_udp_counters_64bit(test));
	if (test->repeating_payload)
	    cJSON_AddNumberToObject(j, "repeating_payload", test->repeating_payload);
#if defined(HAVE_SSL)
	/* Send authentication parameters */
	if (test->settings->client_username && test->settings->client_password && test->settings->client_rsa_pubkey){
	    int rc = encode_auth_setting(test->settings->client_username, test->settings->client_password, test->settings->client_rsa_pubkey, &test->settings->authtoken);

	    if (rc) {
		cJSON_Delete(j);
		i_errno = IESENDPARAMS;
		return -1;
	    }
	    
	    cJSON_AddStringToObject(j, "authtoken", test->settings->authtoken);
	}
#endif // HAVE_SSL
	cJSON_AddStringToObject(j, "client_version", IPERF_VERSION);

	if (test->debug) {
	    char *str = cJSON_Print(j);
	    printf("send_parameters:\n%s\n", str);
	    cJSON_free(str);
	}

	if (JSON_write(test->ctrl_sck, j) < 0) {
	    i_errno = IESENDPARAMS;
	    r = -1;
	}
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
get_parameters(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_p;

    j = JSON_read(test->ctrl_sck);
    if (j == NULL) {
	i_errno = IERECVPARAMS;
        r = -1;
    } else {
	if (test->debug) {
            char *str;
            str = cJSON_Print(j);
            printf("get_parameters:\n%s\n", str );
            cJSON_free(str);
	}

	if ((j_p = cJSON_GetObjectItem(j, "tcp")) != NULL)
	    set_protocol(test, Ptcp);
	if ((j_p = cJSON_GetObjectItem(j, "udp")) != NULL)
	    set_protocol(test, Pudp);
        if ((j_p = cJSON_GetObjectItem(j, "sctp")) != NULL)
            set_protocol(test, Psctp);
	if ((j_p = cJSON_GetObjectItem(j, "omit")) != NULL)
	    test->omit = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "server_affinity")) != NULL)
	    test->server_affinity = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "time")) != NULL)
	    test->duration = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "num")) != NULL)
	    test->settings->bytes = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "blockcount")) != NULL)
	    test->settings->blocks = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "MSS")) != NULL)
	    test->settings->mss = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "nodelay")) != NULL)
	    test->no_delay = 1;
	if ((j_p = cJSON_GetObjectItem(j, "parallel")) != NULL)
	    test->num_streams = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "reverse")) != NULL)
	    iperf_set_test_reverse(test, 1);
        if ((j_p = cJSON_GetObjectItem(j, "bidirectional")) != NULL)
            iperf_set_test_bidirectional(test, 1);
	if ((j_p = cJSON_GetObjectItem(j, "window")) != NULL)
	    test->settings->socket_bufsize = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "len")) != NULL)
	    test->settings->blksize = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "bandwidth")) != NULL)
	    test->settings->rate = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "fqrate")) != NULL)
	    test->settings->fqrate = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "pacing_timer")) != NULL)
	    test->settings->pacing_timer = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "burst")) != NULL)
	    test->settings->burst = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "TOS")) != NULL)
	    test->settings->tos = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "flowlabel")) != NULL)
	    test->settings->flowlabel = j_p->valueint;
	if ((j_p = cJSON_GetObjectItem(j, "title")) != NULL)
	    test->title = strdup(j_p->valuestring);
	if ((j_p = cJSON_GetObjectItem(j, "extra_data")) != NULL)
	    test->extra_data = strdup(j_p->valuestring);
	if ((j_p = cJSON_GetObjectItem(j, "congestion")) != NULL)
	    test->congestion = strdup(j_p->valuestring);
	if ((j_p = cJSON_GetObjectItem(j, "congestion_used")) != NULL)
	    test->congestion_used = strdup(j_p->valuestring);
	if ((j_p = cJSON_GetObjectItem(j, "get_server_output")) != NULL)
	    iperf_set_test_get_server_output(test, 1);
	if ((j_p = cJSON_GetObjectItem(j, "udp_counters_64bit")) != NULL)
	    iperf_set_test_udp_counters_64bit(test, 1);
	if ((j_p = cJSON_GetObjectItem(j, "repeating_payload")) != NULL)
	    test->repeating_payload = 1;
#if defined(HAVE_SSL)
	if ((j_p = cJSON_GetObjectItem(j, "authtoken")) != NULL)
        test->settings->authtoken = strdup(j_p->valuestring);
#endif //HAVE_SSL
	if (test->mode && test->protocol->id == Ptcp && has_tcpinfo_retransmits())
	    test->sender_has_retransmits = 1;
	if (test->settings->rate)
	    cJSON_AddNumberToObject(test->json_start, "target_bitrate", test->settings->rate);
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
send_results(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_streams;
    struct iperf_stream *sp;
    cJSON *j_stream;
    int sender_has_retransmits;
    iperf_size_t bytes_transferred;
    int retransmits;
    struct iperf_time temp_time;
    double start_time, end_time;

    j = cJSON_CreateObject();
    if (j == NULL) {
	i_errno = IEPACKAGERESULTS;
	r = -1;
    } else {
	cJSON_AddNumberToObject(j, "cpu_util_total", test->cpu_util[0]);
	cJSON_AddNumberToObject(j, "cpu_util_user", test->cpu_util[1]);
	cJSON_AddNumberToObject(j, "cpu_util_system", test->cpu_util[2]);
	if ( test->mode == RECEIVER )
	    sender_has_retransmits = -1;
	else
	    sender_has_retransmits = test->sender_has_retransmits;
	cJSON_AddNumberToObject(j, "sender_has_retransmits", sender_has_retransmits);
	if ( test->congestion_used ) {
	    cJSON_AddStringToObject(j, "congestion_used", test->congestion_used);
	}

	/* If on the server and sending server output, then do this */
	if (test->role == 's' && test->get_server_output) {
	    if (test->json_output) {
		/* Add JSON output */
		cJSON_AddItemReferenceToObject(j, "server_output_json", test->json_top);
	    }
	    else {
		/* Add textual output */
		size_t buflen = 0;

		/* Figure out how much room we need to hold the complete output string */
		struct iperf_textline *t;
		TAILQ_FOREACH(t, &(test->server_output_list), textlineentries) {
		    buflen += strlen(t->line);
		}

		/* Allocate and build it up from the component lines */
		char *output = calloc(buflen + 1, 1);
		TAILQ_FOREACH(t, &(test->server_output_list), textlineentries) {
		    strncat(output, t->line, buflen);
		    buflen -= strlen(t->line);
		}

		cJSON_AddStringToObject(j, "server_output_text", output);
        free(output);
	    }
	}

	j_streams = cJSON_CreateArray();
	if (j_streams == NULL) {
	    i_errno = IEPACKAGERESULTS;
	    r = -1;
	} else {
	    cJSON_AddItemToObject(j, "streams", j_streams);
	    SLIST_FOREACH(sp, &test->streams, streams) {
		j_stream = cJSON_CreateObject();
		if (j_stream == NULL) {
		    i_errno = IEPACKAGERESULTS;
		    r = -1;
		} else {
		    cJSON_AddItemToArray(j_streams, j_stream);
		    bytes_transferred = sp->sender ? (sp->result->bytes_sent - sp->result->bytes_sent_omit) : sp->result->bytes_received;
		    retransmits = (sp->sender && test->sender_has_retransmits) ? sp->result->stream_retrans : -1;
		    cJSON_AddNumberToObject(j_stream, "id", sp->id);
		    cJSON_AddNumberToObject(j_stream, "bytes", bytes_transferred);
		    cJSON_AddNumberToObject(j_stream, "retransmits", retransmits);
		    cJSON_AddNumberToObject(j_stream, "jitter", sp->jitter);
		    cJSON_AddNumberToObject(j_stream, "errors", sp->cnt_error);
		    cJSON_AddNumberToObject(j_stream, "packets", sp->packet_count);

		    iperf_time_diff(&sp->result->start_time, &sp->result->start_time, &temp_time);
		    start_time = iperf_time_in_secs(&temp_time);
		    iperf_time_diff(&sp->result->start_time, &sp->result->end_time, &temp_time);
		    end_time = iperf_time_in_secs(&temp_time);
		    cJSON_AddNumberToObject(j_stream, "start_time", start_time);
		    cJSON_AddNumberToObject(j_stream, "end_time", end_time);

		}
	    }
	    if (r == 0 && test->debug) {
                char *str = cJSON_Print(j);
		printf("send_results\n%s\n", str);
                cJSON_free(str);
	    }
	    if (r == 0 && JSON_write(test->ctrl_sck, j) < 0) {
		i_errno = IESENDRESULTS;
		r = -1;
	    }
	}
	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
get_results(struct iperf_test *test)
{
    int r = 0;
    cJSON *j;
    cJSON *j_cpu_util_total;
    cJSON *j_cpu_util_user;
    cJSON *j_cpu_util_system;
    cJSON *j_remote_congestion_used;
    cJSON *j_sender_has_retransmits;
    int result_has_retransmits;
    cJSON *j_streams;
    int n, i;
    cJSON *j_stream;
    cJSON *j_id;
    cJSON *j_bytes;
    cJSON *j_retransmits;
    cJSON *j_jitter;
    cJSON *j_errors;
    cJSON *j_packets;
    cJSON *j_server_output;
    cJSON *j_start_time, *j_end_time;
    int sid, cerror, pcount;
    double jitter;
    iperf_size_t bytes_transferred;
    int retransmits;
    struct iperf_stream *sp;

    j = JSON_read(test->ctrl_sck);
    if (j == NULL) {
	i_errno = IERECVRESULTS;
        r = -1;
    } else {
	j_cpu_util_total = cJSON_GetObjectItem(j, "cpu_util_total");
	j_cpu_util_user = cJSON_GetObjectItem(j, "cpu_util_user");
	j_cpu_util_system = cJSON_GetObjectItem(j, "cpu_util_system");
	j_sender_has_retransmits = cJSON_GetObjectItem(j, "sender_has_retransmits");
	if (j_cpu_util_total == NULL || j_cpu_util_user == NULL || j_cpu_util_system == NULL || j_sender_has_retransmits == NULL) {
	    i_errno = IERECVRESULTS;
	    r = -1;
	} else {
	    if (test->debug) {
                char *str = cJSON_Print(j);
                printf("get_results\n%s\n", str);
                cJSON_free(str);
	    }

	    test->remote_cpu_util[0] = j_cpu_util_total->valuedouble;
	    test->remote_cpu_util[1] = j_cpu_util_user->valuedouble;
	    test->remote_cpu_util[2] = j_cpu_util_system->valuedouble;
	    result_has_retransmits = j_sender_has_retransmits->valueint;
	    if ( test->mode == RECEIVER ) {
	        test->sender_has_retransmits = result_has_retransmits;
	        test->other_side_has_retransmits = 0;
	    }
	    else if ( test->mode == BIDIRECTIONAL )
	        test->other_side_has_retransmits = result_has_retransmits;

	    j_streams = cJSON_GetObjectItem(j, "streams");
	    if (j_streams == NULL) {
		i_errno = IERECVRESULTS;
		r = -1;
	    } else {
	        n = cJSON_GetArraySize(j_streams);
		for (i=0; i<n; ++i) {
		    j_stream = cJSON_GetArrayItem(j_streams, i);
		    if (j_stream == NULL) {
			i_errno = IERECVRESULTS;
			r = -1;
		    } else {
			j_id = cJSON_GetObjectItem(j_stream, "id");
			j_bytes = cJSON_GetObjectItem(j_stream, "bytes");
			j_retransmits = cJSON_GetObjectItem(j_stream, "retransmits");
			j_jitter = cJSON_GetObjectItem(j_stream, "jitter");
			j_errors = cJSON_GetObjectItem(j_stream, "errors");
			j_packets = cJSON_GetObjectItem(j_stream, "packets");
			j_start_time = cJSON_GetObjectItem(j_stream, "start_time");
			j_end_time = cJSON_GetObjectItem(j_stream, "end_time");
			if (j_id == NULL || j_bytes == NULL || j_retransmits == NULL || j_jitter == NULL || j_errors == NULL || j_packets == NULL) {
			    i_errno = IERECVRESULTS;
			    r = -1;
			} else {
			    sid = j_id->valueint;
			    bytes_transferred = j_bytes->valueint;
			    retransmits = j_retransmits->valueint;
			    jitter = j_jitter->valuedouble;
			    cerror = j_errors->valueint;
			    pcount = j_packets->valueint;
			    SLIST_FOREACH(sp, &test->streams, streams)
				if (sp->id == sid) break;
			    if (sp == NULL) {
				i_errno = IESTREAMID;
				r = -1;
			    } else {
				if (sp->sender) {
				    sp->jitter = jitter;
				    sp->cnt_error = cerror;
				    sp->peer_packet_count = pcount;
				    sp->result->bytes_received = bytes_transferred;
				    /*
				     * We have to handle the possibilty that
				     * start_time and end_time might not be
				     * available; this is the case for older (pre-3.2)
				     * servers.
				     *
				     * We need to have result structure members to hold
				     * the both sides' start_time and end_time.
				     */
				    if (j_start_time && j_end_time) {
					sp->result->receiver_time = j_end_time->valuedouble - j_start_time->valuedouble;
				    }
				    else {
					sp->result->receiver_time = 0.0;
				    }
				} else {
				    sp->peer_packet_count = pcount;
				    sp->result->bytes_sent = bytes_transferred;
				    sp->result->stream_retrans = retransmits;
				    if (j_start_time && j_end_time) {
					sp->result->sender_time = j_end_time->valuedouble - j_start_time->valuedouble;
				    }
				    else {
					sp->result->sender_time = 0.0;
				    }
				}
			    }
			}
		    }
		}
		/*
		 * If we're the client and we're supposed to get remote results,
		 * look them up and process accordingly.
		 */
		if (test->role == 'c' && iperf_get_test_get_server_output(test)) {
		    /* Look for JSON.  If we find it, grab the object so it doesn't get deleted. */
		    j_server_output = cJSON_DetachItemFromObject(j, "server_output_json");
		    if (j_server_output != NULL) {
			test->json_server_output = j_server_output;
		    }
		    else {
			/* No JSON, look for textual output.  Make a copy of the text for later. */
			j_server_output = cJSON_GetObjectItem(j, "server_output_text");
			if (j_server_output != NULL) {
			    test->server_output_text = strdup(j_server_output->valuestring);
			}
		    }
		}
	    }
	}

	j_remote_congestion_used = cJSON_GetObjectItem(j, "congestion_used");
	if (j_remote_congestion_used != NULL) {
	    test->remote_congestion_used = strdup(j_remote_congestion_used->valuestring);
	}

	cJSON_Delete(j);
    }
    return r;
}

/*************************************************************/

static int
JSON_write(int fd, cJSON *json)
{
    uint32_t hsize, nsize;
    char *str;
    int r = 0;

    str = cJSON_PrintUnformatted(json);
    if (str == NULL)
	r = -1;
    else {
	hsize = strlen(str);
	nsize = htonl(hsize);
	if (Nwrite(fd, (char*) &nsize, sizeof(nsize), Ptcp) < 0)
	    r = -1;
	else {
	    if (Nwrite(fd, str, hsize, Ptcp) < 0)
		r = -1;
	}
	cJSON_free(str);
    }
    return r;
}

/*************************************************************/

static cJSON *
JSON_read(int fd)
{
    uint32_t hsize, nsize;
    char *str;
    cJSON *json = NULL;
    int rc;

    /*
     * Read a four-byte integer, which is the length of the JSON to follow.
     * Then read the JSON into a buffer and parse it.  Return a parsed JSON
     * structure, NULL if there was an error.
     */
    if (Nread(fd, (char*) &nsize, sizeof(nsize), Ptcp) >= 0) {
	hsize = ntohl(nsize);
	/* Allocate a buffer to hold the JSON */
	str = (char *) calloc(sizeof(char), hsize+1);	/* +1 for trailing null */
	if (str != NULL) {
	    rc = Nread(fd, str, hsize, Ptcp);
	    if (rc >= 0) {
		/*
		 * We should be reading in the number of bytes corresponding to the
		 * length in that 4-byte integer.  If we don't the socket might have
		 * prematurely closed.  Only do the JSON parsing if we got the
		 * correct number of bytes.
		 */
		if (rc == hsize) {
		    json = cJSON_Parse(str);
		}
		else {
		    printf("WARNING:  Size of data read does not correspond to offered length\n");
		}
	    }
	}
	free(str);
    }
    return json;
}

/*************************************************************/
/**
 * add_to_interval_list -- adds new interval to the interval_list
 */

void
add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results * new)
{
    struct iperf_interval_results *irp;

    irp = (struct iperf_interval_results *) malloc(sizeof(struct iperf_interval_results));
    memcpy(irp, new, sizeof(struct iperf_interval_results));
    TAILQ_INSERT_TAIL(&rp->interval_results, irp, irlistentries);
}


/************************************************************/

/**
 * connect_msg -- displays connection message
 * denoting sender/receiver details
 *
 */

void
connect_msg(struct iperf_stream *sp)
{
    char ipl[INET6_ADDRSTRLEN], ipr[INET6_ADDRSTRLEN];
    int lport, rport;

    if (getsockdomain(sp->socket) == AF_INET) {
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &sp->local_addr)->sin_addr, ipl, sizeof(ipl));
	mapped_v4_to_regular_v4(ipl);
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &sp->remote_addr)->sin_addr, ipr, sizeof(ipr));
	mapped_v4_to_regular_v4(ipr);
        lport = ntohs(((struct sockaddr_in *) &sp->local_addr)->sin_port);
        rport = ntohs(((struct sockaddr_in *) &sp->remote_addr)->sin_port);
    } else {
        inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &sp->local_addr)->sin6_addr, ipl, sizeof(ipl));
	mapped_v4_to_regular_v4(ipl);
        inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &sp->remote_addr)->sin6_addr, ipr, sizeof(ipr));
	mapped_v4_to_regular_v4(ipr);
        lport = ntohs(((struct sockaddr_in6 *) &sp->local_addr)->sin6_port);
        rport = ntohs(((struct sockaddr_in6 *) &sp->remote_addr)->sin6_port);
    }

    if (sp->test->json_output)
        cJSON_AddItemToArray(sp->test->json_connected, iperf_json_printf("socket: %d  local_host: %s  local_port: %d  remote_host: %s  remote_port: %d", (int64_t) sp->socket, ipl, (int64_t) lport, ipr, (int64_t) rport));
    else
	iperf_printf(sp->test, report_connected, sp->socket, ipl, lport, ipr, rport);
}


/**************************************************************************/

struct iperf_test *
iperf_new_test()
{
    struct iperf_test *test;

    test = (struct iperf_test *) malloc(sizeof(struct iperf_test));
    if (!test) {
        i_errno = IENEWTEST;
        return NULL;
    }
    /* initialize everything to zero */
    memset(test, 0, sizeof(struct iperf_test));

    test->settings = (struct iperf_settings *) malloc(sizeof(struct iperf_settings));
    if (!test->settings) {
        free(test);
	i_errno = IENEWTEST;
	return NULL;
    }
    memset(test->settings, 0, sizeof(struct iperf_settings));

    /* By default all output goes to stdout */
    test->outfile = stdout;

    return test;
}

/**************************************************************************/

struct protocol *
protocol_new(void)
{
    struct protocol *proto;

    proto = malloc(sizeof(struct protocol));
    if(!proto) {
        return NULL;
    }
    memset(proto, 0, sizeof(struct protocol));

    return proto;
}

void
protocol_free(struct protocol *proto)
{
    free(proto); 
}

/**************************************************************************/
int
iperf_defaults(struct iperf_test *testp)
{
    struct protocol *tcp, *udp;
#if defined(HAVE_SCTP)
    struct protocol *sctp;
#endif /* HAVE_SCTP */

    testp->omit = OMIT;
    testp->duration = DURATION;
    testp->diskfile_name = (char*) 0;
    testp->affinity = -1;
    testp->server_affinity = -1;
    TAILQ_INIT(&testp->xbind_addrs);
#if defined(HAVE_CPUSET_SETAFFINITY)
    CPU_ZERO(&testp->cpumask);
#endif /* HAVE_CPUSET_SETAFFINITY */
    testp->title = NULL;
    testp->extra_data = NULL;
    testp->congestion = NULL;
    testp->congestion_used = NULL;
    testp->remote_congestion_used = NULL;
    testp->server_port = PORT;
    testp->ctrl_sck = -1;
    testp->prot_listener = -1;
    testp->other_side_has_retransmits = 0;

    testp->stats_callback = iperf_stats_callback;
    testp->reporter_callback = iperf_reporter_callback;

    testp->stats_interval = testp->reporter_interval = 1;
    testp->num_streams = 1;

    testp->settings->domain = AF_UNSPEC;
    testp->settings->unit_format = 'a';
    testp->settings->socket_bufsize = 0;    /* use autotuning */
    testp->settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->settings->rate = 0;
    testp->settings->fqrate = 0;
    testp->settings->pacing_timer = 1000;
    testp->settings->burst = 0;
    testp->settings->mss = 0;
    testp->settings->bytes = 0;
    testp->settings->blocks = 0;
    testp->settings->connect_timeout = -1;
    memset(testp->cookie, 0, COOKIE_SIZE);

    testp->multisend = 10;	/* arbitrary */

    /* Set up protocol list */
    SLIST_INIT(&testp->streams);
    SLIST_INIT(&testp->protocols);

    tcp = protocol_new();
    if (!tcp)
        return -1;

    tcp->id = Ptcp;
    tcp->name = "TCP";
    tcp->accept = iperf_tcp_accept;
    tcp->listen = iperf_tcp_listen;
    tcp->connect = iperf_tcp_connect;
    tcp->send = iperf_tcp_send;
    tcp->recv = iperf_tcp_recv;
    tcp->init = NULL;
    SLIST_INSERT_HEAD(&testp->protocols, tcp, protocols);

    udp = protocol_new();
    if (!udp) {
        protocol_free(tcp);
        return -1;
    }

    udp->id = Pudp;
    udp->name = "UDP";
    udp->accept = iperf_udp_accept;
    udp->listen = iperf_udp_listen;
    udp->connect = iperf_udp_connect;
    udp->send = iperf_udp_send;
    udp->recv = iperf_udp_recv;
    udp->init = iperf_udp_init;
    SLIST_INSERT_AFTER(tcp, udp, protocols);

    set_protocol(testp, Ptcp);

#if defined(HAVE_SCTP)
    sctp = protocol_new();
    if (!sctp) {
        protocol_free(tcp);
        protocol_free(udp);
        return -1;
    }

    sctp->id = Psctp;
    sctp->name = "SCTP";
    sctp->accept = iperf_sctp_accept;
    sctp->listen = iperf_sctp_listen;
    sctp->connect = iperf_sctp_connect;
    sctp->send = iperf_sctp_send;
    sctp->recv = iperf_sctp_recv;
    sctp->init = iperf_sctp_init;

    SLIST_INSERT_AFTER(udp, sctp, protocols);
#endif /* HAVE_SCTP */

    testp->on_new_stream = iperf_on_new_stream;
    testp->on_test_start = iperf_on_test_start;
    testp->on_connect = iperf_on_connect;
    testp->on_test_finish = iperf_on_test_finish;

    TAILQ_INIT(&testp->server_output_list);

    return 0;
}


/**************************************************************************/
void
iperf_free_test(struct iperf_test *test)
{
    struct protocol *prot;
    struct iperf_stream *sp;

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }
    if (test->server_hostname)
	free(test->server_hostname);
    if (test->tmp_template)
	free(test->tmp_template);
    if (test->bind_address)
	free(test->bind_address);
    if (!TAILQ_EMPTY(&test->xbind_addrs)) {
        struct xbind_entry *xbe;

        while (!TAILQ_EMPTY(&test->xbind_addrs)) {
            xbe = TAILQ_FIRST(&test->xbind_addrs);
            TAILQ_REMOVE(&test->xbind_addrs, xbe, link);
            if (xbe->ai)
                freeaddrinfo(xbe->ai);
            free(xbe->name);
            free(xbe);
        }
    }
#if defined(HAVE_SSL)

    if (test->server_rsa_private_key)
      EVP_PKEY_free(test->server_rsa_private_key);
    test->server_rsa_private_key = NULL;

    free(test->settings->authtoken);
    test->settings->authtoken = NULL;

    free(test->settings->client_username);
    test->settings->client_username = NULL;

    free(test->settings->client_password);
    test->settings->client_password = NULL;

    if (test->settings->client_rsa_pubkey)
      EVP_PKEY_free(test->settings->client_rsa_pubkey);
    test->settings->client_rsa_pubkey = NULL;
#endif /* HAVE_SSL */

    if (test->settings)
    free(test->settings);
    if (test->title)
	free(test->title);
    if (test->extra_data)
	free(test->extra_data);
    if (test->congestion)
	free(test->congestion);
    if (test->congestion_used)
	free(test->congestion_used);
    if (test->remote_congestion_used)
	free(test->remote_congestion_used);
    if (test->omit_timer != NULL)
	tmr_cancel(test->omit_timer);
    if (test->timer != NULL)
	tmr_cancel(test->timer);
    if (test->stats_timer != NULL)
	tmr_cancel(test->stats_timer);
    if (test->reporter_timer != NULL)
	tmr_cancel(test->reporter_timer);

    /* Free protocol list */
    while (!SLIST_EMPTY(&test->protocols)) {
        prot = SLIST_FIRST(&test->protocols);
        SLIST_REMOVE_HEAD(&test->protocols, protocols);        
        free(prot);
    }

    if (test->server_output_text) {
	free(test->server_output_text);
	test->server_output_text = NULL;
    }

    if (test->json_output_string) {
	free(test->json_output_string);
	test->json_output_string = NULL;
    }

    /* Free output line buffers, if any (on the server only) */
    struct iperf_textline *t;
    while (!TAILQ_EMPTY(&test->server_output_list)) {
	t = TAILQ_FIRST(&test->server_output_list);
	TAILQ_REMOVE(&test->server_output_list, t, textlineentries);
	free(t->line);
	free(t);
    }

    /* sctp_bindx: do not free the arguments, only the resolver results */
    if (!TAILQ_EMPTY(&test->xbind_addrs)) {
        struct xbind_entry *xbe;

        TAILQ_FOREACH(xbe, &test->xbind_addrs, link) {
            if (xbe->ai) {
                freeaddrinfo(xbe->ai);
                xbe->ai = NULL;
            }
        }
    }

    /* XXX: Why are we setting these values to NULL? */
    // test->streams = NULL;
    test->stats_callback = NULL;
    test->reporter_callback = NULL;
    free(test);
}


void
iperf_reset_test(struct iperf_test *test)
{
    struct iperf_stream *sp;

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }
    if (test->omit_timer != NULL) {
	tmr_cancel(test->omit_timer);
	test->omit_timer = NULL;
    }
    if (test->timer != NULL) {
	tmr_cancel(test->timer);
	test->timer = NULL;
    }
    if (test->stats_timer != NULL) {
	tmr_cancel(test->stats_timer);
	test->stats_timer = NULL;
    }
    if (test->reporter_timer != NULL) {
	tmr_cancel(test->reporter_timer);
	test->reporter_timer = NULL;
    }
    test->done = 0;

    SLIST_INIT(&test->streams);

    if (test->remote_congestion_used)
        free(test->remote_congestion_used);
    test->remote_congestion_used = NULL;
    test->role = 's';
    test->mode = RECEIVER;
    test->sender_has_retransmits = 0;
    set_protocol(test, Ptcp);
    test->omit = OMIT;
    test->duration = DURATION;
    test->server_affinity = -1;
#if defined(HAVE_CPUSET_SETAFFINITY)
    CPU_ZERO(&test->cpumask);
#endif /* HAVE_CPUSET_SETAFFINITY */
    test->state = 0;
    
    test->ctrl_sck = -1;
    test->prot_listener = -1;

    test->bytes_sent = 0;
    test->blocks_sent = 0;

    test->bytes_received = 0;
    test->blocks_received = 0;

    test->other_side_has_retransmits = 0;

    test->reverse = 0;
    test->bidirectional = 0;
    test->no_delay = 0;

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = 0;
    test->settings->burst = 0;
    test->settings->mss = 0;
    test->settings->tos = 0;

#if defined(HAVE_SSL)
    if (test->settings->authtoken) {
        free(test->settings->authtoken);
        test->settings->authtoken = NULL;
    }
    if (test->settings->client_username) {
        free(test->settings->client_username);
        test->settings->client_username = NULL;
    }
    if (test->settings->client_password) {
        free(test->settings->client_password);
        test->settings->client_password = NULL;
    }
    if (test->settings->client_rsa_pubkey) {
        EVP_PKEY_free(test->settings->client_rsa_pubkey);
        test->settings->client_rsa_pubkey = NULL;
    }
#endif /* HAVE_SSL */

    memset(test->cookie, 0, COOKIE_SIZE);
    test->multisend = 10;	/* arbitrary */
    test->udp_counters_64bit = 0;
    if (test->title) {
	free(test->title);
	test->title = NULL;
    }
    if (test->extra_data) {
	free(test->extra_data);
	test->extra_data = NULL;
    }

    /* Free output line buffers, if any (on the server only) */
    struct iperf_textline *t;
    while (!TAILQ_EMPTY(&test->server_output_list)) {
	t = TAILQ_FIRST(&test->server_output_list);
	TAILQ_REMOVE(&test->server_output_list, t, textlineentries);
	free(t->line);
	free(t);
    }
}


/* Reset all of a test's stats back to zero.  Called when the omitting
** period is over.
*/
void
iperf_reset_stats(struct iperf_test *test)
{
    struct iperf_time now;
    struct iperf_stream *sp;
    struct iperf_stream_result *rp;

    test->bytes_sent = 0;
    test->blocks_sent = 0;
    iperf_time_now(&now);
    SLIST_FOREACH(sp, &test->streams, streams) {
	sp->omitted_packet_count = sp->packet_count;
        sp->omitted_cnt_error = sp->cnt_error;
        sp->omitted_outoforder_packets = sp->outoforder_packets;
	sp->jitter = 0;
	rp = sp->result;
        rp->bytes_sent_omit = rp->bytes_sent;
        rp->bytes_received = 0;
        rp->bytes_sent_this_interval = rp->bytes_received_this_interval = 0;
	if (test->sender_has_retransmits == 1) {
	    struct iperf_interval_results ir; /* temporary results structure */
	    save_tcpinfo(sp, &ir);
	    rp->stream_prev_total_retrans = get_total_retransmits(&ir);
	}
	rp->stream_retrans = 0;
	rp->start_time = now;
    }
}


/**************************************************************************/

/**
 * Gather statistics during a test.
 * This function works for both the client and server side.
 */
void
iperf_stats_callback(struct iperf_test *test)
{
    struct iperf_stream *sp;
    struct iperf_stream_result *rp = NULL;
    struct iperf_interval_results *irp, temp;
    struct iperf_time temp_time;

    temp.omitted = test->omitting;
    SLIST_FOREACH(sp, &test->streams, streams) {
        rp = sp->result;
	temp.bytes_transferred = sp->sender ? rp->bytes_sent_this_interval : rp->bytes_received_this_interval;
     
	irp = TAILQ_LAST(&rp->interval_results, irlisthead);
        /* result->end_time contains timestamp of previous interval */
        if ( irp != NULL ) /* not the 1st interval */
            memcpy(&temp.interval_start_time, &rp->end_time, sizeof(struct iperf_time));
        else /* or use timestamp from beginning */
            memcpy(&temp.interval_start_time, &rp->start_time, sizeof(struct iperf_time));
        /* now save time of end of this interval */
        iperf_time_now(&rp->end_time);
        memcpy(&temp.interval_end_time, &rp->end_time, sizeof(struct iperf_time));
        iperf_time_diff(&temp.interval_start_time, &temp.interval_end_time, &temp_time);
        temp.interval_duration = iperf_time_in_secs(&temp_time);
	if (test->protocol->id == Ptcp) {
	    if ( has_tcpinfo()) {
		save_tcpinfo(sp, &temp);
		if (test->sender_has_retransmits == 1) {
		    long total_retrans = get_total_retransmits(&temp);
		    temp.interval_retrans = total_retrans - rp->stream_prev_total_retrans;
		    rp->stream_retrans += temp.interval_retrans;
		    rp->stream_prev_total_retrans = total_retrans;

		    temp.snd_cwnd = get_snd_cwnd(&temp);
		    if (temp.snd_cwnd > rp->stream_max_snd_cwnd) {
			rp->stream_max_snd_cwnd = temp.snd_cwnd;
		    }
		    
		    temp.rtt = get_rtt(&temp);
		    if (temp.rtt > rp->stream_max_rtt) {
			rp->stream_max_rtt = temp.rtt;
		    }
		    if (rp->stream_min_rtt == 0 ||
			temp.rtt < rp->stream_min_rtt) {
			rp->stream_min_rtt = temp.rtt;
		    }
		    rp->stream_sum_rtt += temp.rtt;
		    rp->stream_count_rtt++;

		    temp.rttvar = get_rttvar(&temp);
		    temp.pmtu = get_pmtu(&temp);
		}
	    }
	} else {
	    if (irp == NULL) {
		temp.interval_packet_count = sp->packet_count;
		temp.interval_outoforder_packets = sp->outoforder_packets;
		temp.interval_cnt_error = sp->cnt_error;
	    } else {
		temp.interval_packet_count = sp->packet_count - irp->packet_count;
		temp.interval_outoforder_packets = sp->outoforder_packets - irp->outoforder_packets;
		temp.interval_cnt_error = sp->cnt_error - irp->cnt_error;
	    }
	    temp.packet_count = sp->packet_count;
	    temp.jitter = sp->jitter;
	    temp.outoforder_packets = sp->outoforder_packets;
	    temp.cnt_error = sp->cnt_error;
	}
        add_to_interval_list(rp, &temp);
        rp->bytes_sent_this_interval = rp->bytes_received_this_interval = 0;
    }
}

/**
 * Print intermediate results during a test (interval report).
 * Uses print_interval_results to print the results for each stream,
 * then prints an interval summary for all streams in this
 * interval.
 */
static void
iperf_print_intermediate(struct iperf_test *test)
{
    struct iperf_stream *sp = NULL;
    struct iperf_interval_results *irp;
    struct iperf_time temp_time;
    cJSON *json_interval;
    cJSON *json_interval_streams;

    int lower_mode, upper_mode;
    int current_mode;

    /*
     * Due to timing oddities, there can be cases, especially on the
     * server side, where at the end of a test there is a fairly short
     * interval with no data transferred.  This could caused by
     * the control and data flows sharing the same path in the network,
     * and having the control messages for stopping the test being
     * queued behind the data packets.
     *
     * We'd like to try to omit that last interval when it happens, to
     * avoid cluttering data and output with useless stuff.
     * So we're going to try to ignore very short intervals (less than
     * 10% of the interval time) that have no data.
     */
    int interval_ok = 0;
    SLIST_FOREACH(sp, &test->streams, streams) {
	irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);
	if (irp) {
	    iperf_time_diff(&irp->interval_start_time, &irp->interval_end_time, &temp_time);
	    double interval_len = iperf_time_in_secs(&temp_time);
	    if (test->debug) {
		printf("interval_len %f bytes_transferred %" PRIu64 "\n", interval_len, irp->bytes_transferred);
	    }

	    /*
	     * If the interval is at least 10% the normal interval
	     * length, or if there were actual bytes transferrred,
	     * then we want to keep this interval.
	     */
	    if (interval_len >= test->stats_interval * 0.10 ||
		irp->bytes_transferred > 0) {
		interval_ok = 1;
		if (test->debug) {
		    printf("interval forces keep\n");
		}
	    }
	}
    }
    if (!interval_ok) {
	if (test->debug) {
	    printf("ignoring short interval with no data\n");
	}
	return;
    }

    if (test->json_output) {
        json_interval = cJSON_CreateObject();
	if (json_interval == NULL)
	    return;
	cJSON_AddItemToArray(test->json_intervals, json_interval);
        json_interval_streams = cJSON_CreateArray();
	if (json_interval_streams == NULL)
	    return;
	cJSON_AddItemToObject(json_interval, "streams", json_interval_streams);
    } else {
        json_interval = NULL;
        json_interval_streams = NULL;
    }

    /*
     * We must to sum streams separately.
     * For bidirectional mode we must to display
     * information about sender and receiver streams.
     * For client side we must handle sender streams
     * firstly and receiver streams for server side.
     * The following design allows us to do this.
     */

    if (test->mode == BIDIRECTIONAL) {
        if (test->role == 'c') {
            lower_mode = -1;
            upper_mode = 0;
        } else {
            lower_mode = 0;
            upper_mode = 1;
        }
    } else {
        lower_mode = test->mode;
        upper_mode = lower_mode;
    }


    for (current_mode = lower_mode; current_mode <= upper_mode; ++current_mode) {
        char ubuf[UNIT_LEN];
        char nbuf[UNIT_LEN];
        char mbuf[UNIT_LEN];
        char zbuf[] = "          ";

        iperf_size_t bytes = 0;
        double bandwidth;
        int retransmits = 0;
        double start_time, end_time;

        int total_packets = 0, lost_packets = 0;
        double avg_jitter = 0.0, lost_percent;
        int stream_must_be_sender = current_mode * current_mode;

        /*  Print stream role just for bidirectional mode. */

        if (test->mode == BIDIRECTIONAL) {
            sprintf(mbuf, "[%s-%s]", stream_must_be_sender?"TX":"RX", test->role == 'c'?"C":"S");
        } else {
            mbuf[0] = '\0';
            zbuf[0] = '\0';
        }

        SLIST_FOREACH(sp, &test->streams, streams) {
            if (sp->sender == stream_must_be_sender) {
                print_interval_results(test, sp, json_interval_streams);
                /* sum up all streams */
                irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);
                if (irp == NULL) {
                    iperf_err(test,
                            "iperf_print_intermediate error: interval_results is NULL");
                    return;
                }
                bytes += irp->bytes_transferred;
                if (test->protocol->id == Ptcp) {
                    if (test->sender_has_retransmits == 1) {
                        retransmits += irp->interval_retrans;
                    }
                } else {
                    total_packets += irp->interval_packet_count;
                    lost_packets += irp->interval_cnt_error;
                    avg_jitter += irp->jitter;
                }
            }
        }

        /* next build string with sum of all streams */
        if (test->num_streams > 1 || test->json_output) {
            sp = SLIST_FIRST(&test->streams); /* reset back to 1st stream */
            /* Only do this of course if there was a first stream */
            if (sp) {
	    irp = TAILQ_LAST(&sp->result->interval_results, irlisthead);    /* use 1st stream for timing info */

	    unit_snprintf(ubuf, UNIT_LEN, (double) bytes, 'A');
	    bandwidth = (double) bytes / (double) irp->interval_duration;
	    unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);

	    iperf_time_diff(&sp->result->start_time,&irp->interval_start_time, &temp_time);
	    start_time = iperf_time_in_secs(&temp_time);
	    iperf_time_diff(&sp->result->start_time,&irp->interval_end_time, &temp_time);
	    end_time = iperf_time_in_secs(&temp_time);
                if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
                    if (test->sender_has_retransmits == 1 && stream_must_be_sender) {
                        /* Interval sum, TCP with retransmits. */
                        if (test->json_output)
                            cJSON_AddItemToObject(json_interval, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  retransmits: %d  omitted: %b sender: %b", (double) start_time, (double) end_time, (double) irp->interval_duration, (int64_t) bytes, bandwidth * 8, (int64_t) retransmits, irp->omitted, stream_must_be_sender)); /* XXX irp->omitted or test->omitting? */
                        else
                            iperf_printf(test, report_sum_bw_retrans_format, mbuf, start_time, end_time, ubuf, nbuf, retransmits, irp->omitted?report_omitted:""); /* XXX irp->omitted or test->omitting? */
                    } else {
                        /* Interval sum, TCP without retransmits. */
                        if (test->json_output)
                            cJSON_AddItemToObject(json_interval, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  omitted: %b sender: %b", (double) start_time, (double) end_time, (double) irp->interval_duration, (int64_t) bytes, bandwidth * 8, test->omitting, stream_must_be_sender));
                        else
                            iperf_printf(test, report_sum_bw_format, mbuf, start_time, end_time, ubuf, nbuf, test->omitting?report_omitted:"");
                    }
                } else {
                    /* Interval sum, UDP. */
                    if (stream_must_be_sender) {
                        if (test->json_output)
                            cJSON_AddItemToObject(json_interval, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  packets: %d  omitted: %b sender: %b", (double) start_time, (double) end_time, (double) irp->interval_duration, (int64_t) bytes, bandwidth * 8, (int64_t) total_packets, test->omitting, stream_must_be_sender));
                        else
                            iperf_printf(test, report_sum_bw_udp_sender_format, mbuf, start_time, end_time, ubuf, nbuf, zbuf, total_packets, test->omitting?report_omitted:"");
                    } else {
                        avg_jitter /= test->num_streams;
                        if (total_packets > 0) {
                            lost_percent = 100.0 * lost_packets / total_packets;
                        }
                        else {
                            lost_percent = 0.0;
                        }
                        if (test->json_output)
                            cJSON_AddItemToObject(json_interval, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f  omitted: %b sender: %b", (double) start_time, (double) end_time, (double) irp->interval_duration, (int64_t) bytes, bandwidth * 8, (double) avg_jitter * 1000.0, (int64_t) lost_packets, (int64_t) total_packets, (double) lost_percent, test->omitting, stream_must_be_sender));
                        else
                            iperf_printf(test, report_sum_bw_udp_format, mbuf, start_time, end_time, ubuf, nbuf, avg_jitter * 1000.0, lost_packets, total_packets, lost_percent, test->omitting?report_omitted:"");
                    }
                }
            }
        }
    }
}

/**
 * Print overall summary statistics at the end of a test.
 */
static void
iperf_print_results(struct iperf_test *test)
{

    cJSON *json_summary_streams = NULL;

    int lower_mode, upper_mode;
    int current_mode;

    int tmp_sender_has_retransmits = test->sender_has_retransmits;

    /* print final summary for all intervals */

    if (test->json_output) {
        json_summary_streams = cJSON_CreateArray();
	if (json_summary_streams == NULL)
	    return;
	cJSON_AddItemToObject(test->json_end, "streams", json_summary_streams);
    } else {
	iperf_printf(test, "%s", report_bw_separator);
	if (test->verbose)
	    iperf_printf(test, "%s", report_summary);
	if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
	    if (test->sender_has_retransmits || test->other_side_has_retransmits) {
	        if (test->bidirectional)
	            iperf_printf(test, "%s", report_bw_retrans_header_bidir);
	        else
	            iperf_printf(test, "%s", report_bw_retrans_header);
	    }
	    else {
	        if (test->bidirectional)
	            iperf_printf(test, "%s", report_bw_header_bidir);
	        else
	            iperf_printf(test, "%s", report_bw_header);
	    }
	} else {
	    if (test->bidirectional)
	        iperf_printf(test, "%s", report_bw_udp_header_bidir);
	    else
	        iperf_printf(test, "%s", report_bw_udp_header);
	}
    }

    /*
     * We must to sum streams separately.
     * For bidirectional mode we must to display
     * information about sender and receiver streams.
     * For client side we must handle sender streams
     * firstly and receiver streams for server side.
     * The following design allows us to do this.
     */

    if (test->mode == BIDIRECTIONAL) {
        if (test->role == 'c') {
            lower_mode = -1;
            upper_mode = 0;
        } else {
            lower_mode = 0;
            upper_mode = 1;
        }
    } else {
        lower_mode = test->mode;
        upper_mode = lower_mode;
    }


    for (current_mode = lower_mode; current_mode <= upper_mode; ++current_mode) {
        cJSON *json_summary_stream = NULL;
        int total_retransmits = 0;
        int total_packets = 0, lost_packets = 0;
        int sender_packet_count = 0, receiver_packet_count = 0; /* for this stream, this interval */
        int sender_total_packets = 0, receiver_total_packets = 0; /* running total */
        char ubuf[UNIT_LEN];
        char nbuf[UNIT_LEN];
        struct stat sb;
        char sbuf[UNIT_LEN];
        struct iperf_stream *sp = NULL;
        iperf_size_t bytes_sent, total_sent = 0;
        iperf_size_t bytes_received, total_received = 0;
        double start_time, end_time = 0.0, avg_jitter = 0.0, lost_percent = 0.0;
        double sender_time = 0.0, receiver_time = 0.0;
    struct iperf_time temp_time;
        double bandwidth;

        char mbuf[UNIT_LEN];
        int stream_must_be_sender = current_mode * current_mode;


        /*  Print stream role just for bidirectional mode. */

        if (test->mode == BIDIRECTIONAL) {
            sprintf(mbuf, "[%s-%s]", stream_must_be_sender?"TX":"RX", test->role == 'c'?"C":"S");
        } else {
            mbuf[0] = '\0';
        }

        /* Get sender_has_retransmits for each sender side (client and server) */
        if (test->mode == BIDIRECTIONAL && stream_must_be_sender)
            test->sender_has_retransmits = tmp_sender_has_retransmits;
        else if (test->mode == BIDIRECTIONAL && !stream_must_be_sender)
            test->sender_has_retransmits = test->other_side_has_retransmits;

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
    iperf_time_diff(&sp->result->start_time, &sp->result->end_time, &temp_time);
    end_time = iperf_time_in_secs(&temp_time);
        if (sp->sender) {
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
            if (sp->sender == stream_must_be_sender) {
                if (test->json_output) {
                    json_summary_stream = cJSON_CreateObject();
                    if (json_summary_stream == NULL)
                        return;
                    cJSON_AddItemToArray(json_summary_streams, json_summary_stream);
                }

                bytes_sent = sp->result->bytes_sent - sp->result->bytes_sent_omit;
                bytes_received = sp->result->bytes_received;
                total_sent += bytes_sent;
                total_received += bytes_received;

                if (sp->sender) {
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
                } else {
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

                unit_snprintf(ubuf, UNIT_LEN, (double) bytes_sent, 'A');
                if (sender_time > 0.0) {
                    bandwidth = (double) bytes_sent / (double) sender_time;
                }
                else {
                    bandwidth = 0.0;
                }
                unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
                if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
                    if (test->sender_has_retransmits) {
                        /* Sender summary, TCP and SCTP with retransmits. */
                        if (test->json_output)
                            cJSON_AddItemToObject(json_summary_stream, "sender", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  retransmits: %d  max_snd_cwnd:  %d  max_rtt:  %d  min_rtt:  %d  mean_rtt:  %d sender: %b", (int64_t) sp->socket, (double) start_time, (double) sender_time, (double) sender_time, (int64_t) bytes_sent, bandwidth * 8, (int64_t) sp->result->stream_retrans, (int64_t) sp->result->stream_max_snd_cwnd, (int64_t) sp->result->stream_max_rtt, (int64_t) sp->result->stream_min_rtt, (int64_t) ((sp->result->stream_count_rtt == 0) ? 0 : sp->result->stream_sum_rtt / sp->result->stream_count_rtt), stream_must_be_sender));
                        else
                            if (test->role == 's' && !sp->sender) {
                                if (test->verbose)
                                    iperf_printf(test, report_sender_not_available_format, sp->socket);
                            }
                            else {
                                iperf_printf(test, report_bw_retrans_format, sp->socket, mbuf, start_time, sender_time, ubuf, nbuf, sp->result->stream_retrans, report_sender);
                            }
                    } else {
                        /* Sender summary, TCP and SCTP without retransmits. */
                        if (test->json_output)
                            cJSON_AddItemToObject(json_summary_stream, "sender", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f sender: %b", (int64_t) sp->socket, (double) start_time, (double) sender_time, (double) sender_time, (int64_t) bytes_sent, bandwidth * 8,  stream_must_be_sender));
                        else
                            if (test->role == 's' && !sp->sender) {
                                if (test->verbose)
                                    iperf_printf(test, report_sender_not_available_format, sp->socket);
                            }
                            else {
                                iperf_printf(test, report_bw_format, sp->socket, mbuf, start_time, sender_time, ubuf, nbuf, report_sender);
                            }
                    }
                } else {
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
                        cJSON_AddItemToObject(json_summary_stream, "udp", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f  out_of_order: %d sender: %b", (int64_t) sp->socket, (double) start_time, (double) sender_time, (double) sender_time, (int64_t) bytes_sent, bandwidth * 8, (double) sp->jitter * 1000.0, (int64_t) (sp->cnt_error - sp->omitted_cnt_error), (int64_t) (packet_count - sp->omitted_packet_count), (double) lost_percent, (int64_t) (sp->outoforder_packets - sp->omitted_outoforder_packets), stream_must_be_sender));
                    }
                    else {
                        /*
                         * Due to ordering of messages on the control channel,
                         * the server cannot report on client-side summary
                         * statistics.  If we're the server, omit one set of
                         * summary statistics to avoid giving meaningless
                         * results.
                         */
                        if (test->role == 's' && !sp->sender) {
                            if (test->verbose)
                                iperf_printf(test, report_sender_not_available_format, sp->socket);
                        }
                        else {
                            iperf_printf(test, report_bw_udp_format, sp->socket, mbuf, start_time, sender_time, ubuf, nbuf, 0.0, 0, (sender_packet_count - sp->omitted_packet_count), (double) 0, report_sender);
                        }
                        if ((sp->outoforder_packets - sp->omitted_outoforder_packets) > 0)
                          iperf_printf(test, report_sum_outoforder, mbuf, start_time, sender_time, (sp->outoforder_packets - sp->omitted_outoforder_packets));
                    }
                }

                if (sp->diskfile_fd >= 0) {
                    if (fstat(sp->diskfile_fd, &sb) == 0) {
                        /* In the odd case that it's a zero-sized file, say it was all transferred. */
                        int percent_sent = 100, percent_received = 100;
                        if (sb.st_size > 0) {
                            percent_sent = (int) ( ( (double) bytes_sent / (double) sb.st_size ) * 100.0 );
                            percent_received = (int) ( ( (double) bytes_received / (double) sb.st_size ) * 100.0 );
                        }
                        unit_snprintf(sbuf, UNIT_LEN, (double) sb.st_size, 'A');
                        if (test->json_output)
                            cJSON_AddItemToObject(json_summary_stream, "diskfile", iperf_json_printf("sent: %d  received: %d  size: %d  percent_sent: %d  percent_received: %d  filename: %s", (int64_t) bytes_sent, (int64_t) bytes_received, (int64_t) sb.st_size, (int64_t) percent_sent, (int64_t) percent_received, test->diskfile_name));
                        else
                            if (stream_must_be_sender) {
                                iperf_printf(test, report_diskfile, ubuf, sbuf, percent_sent, test->diskfile_name);
                            }
                            else {
                                unit_snprintf(ubuf, UNIT_LEN, (double) bytes_received, 'A');
                                iperf_printf(test, report_diskfile, ubuf, sbuf, percent_received, test->diskfile_name);
                            }
                    }
                }

                unit_snprintf(ubuf, UNIT_LEN, (double) bytes_received, 'A');
                if (receiver_time > 0) {
                    bandwidth = (double) bytes_received / (double) receiver_time;
                }
                else {
                    bandwidth = 0.0;
                }
                unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
                if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
                    /* Receiver summary, TCP and SCTP */
                    if (test->json_output)
                        cJSON_AddItemToObject(json_summary_stream, "receiver", iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f sender: %b", (int64_t) sp->socket, (double) start_time, (double) receiver_time, (double) end_time, (int64_t) bytes_received, bandwidth * 8, stream_must_be_sender));
                    else
                        if (test->role == 's' && sp->sender) {
                            if (test->verbose)
                                iperf_printf(test, report_receiver_not_available_format, sp->socket);
                        }
                        else {
                            iperf_printf(test, report_bw_format, sp->socket, mbuf, start_time, receiver_time, ubuf, nbuf, report_receiver);
                        }
                }
                else {
                    /*
                     * Receiver summary, UDP.  Note that JSON was emitted with
                     * the sender summary, so we only deal with human-readable
                     * data here.
                     */
                    if (! test->json_output) {
                        if (receiver_packet_count - sp->omitted_packet_count > 0) {
                            lost_percent = 100.0 * (sp->cnt_error - sp->omitted_cnt_error) / (receiver_packet_count - sp->omitted_packet_count);
                        }
                        else {
                            lost_percent = 0.0;
                        }

                        if (test->role == 's' && sp->sender) {
                            if (test->verbose)
                                iperf_printf(test, report_receiver_not_available_format, sp->socket);
                        }
                        else {
                            iperf_printf(test, report_bw_udp_format, sp->socket, mbuf, start_time, receiver_time, ubuf, nbuf, sp->jitter * 1000.0, (sp->cnt_error - sp->omitted_cnt_error), (receiver_packet_count - sp->omitted_packet_count), lost_percent, report_receiver);
                        }
                    }
                }
            }
        }
        }

        if (test->num_streams > 1 || test->json_output) {
            unit_snprintf(ubuf, UNIT_LEN, (double) total_sent, 'A');
            /* If no tests were run, arbitrarily set bandwidth to 0. */
            if (sender_time > 0.0) {
                bandwidth = (double) total_sent / (double) sender_time;
            }
            else {
                bandwidth = 0.0;
            }
            unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
            if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
                if (test->sender_has_retransmits) {
                    /* Summary sum, TCP with retransmits. */
                    if (test->json_output)
                        cJSON_AddItemToObject(test->json_end, "sum_sent", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  retransmits: %d sender: %b", (double) start_time, (double) sender_time, (double) sender_time, (int64_t) total_sent, bandwidth * 8, (int64_t) total_retransmits, stream_must_be_sender));
                    else
                        if (test->role == 's' && !stream_must_be_sender) {
                            if (test->verbose)
                                iperf_printf(test, report_sender_not_available_summary_format, "SUM");
                        }
                        else {
                          iperf_printf(test, report_sum_bw_retrans_format, mbuf, start_time, sender_time, ubuf, nbuf, total_retransmits, report_sender);
                        }
                } else {
                    /* Summary sum, TCP without retransmits. */
                    if (test->json_output)
                        cJSON_AddItemToObject(test->json_end, "sum_sent", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f sender: %b", (double) start_time, (double) sender_time, (double) sender_time, (int64_t) total_sent, bandwidth * 8, stream_must_be_sender));
                    else
                        if (test->role == 's' && !stream_must_be_sender) {
                            if (test->verbose)
                                iperf_printf(test, report_sender_not_available_summary_format, "SUM");
                        }
                        else {
                            iperf_printf(test, report_sum_bw_format, mbuf, start_time, sender_time, ubuf, nbuf, report_sender);
                        }
                }
                unit_snprintf(ubuf, UNIT_LEN, (double) total_received, 'A');
                /* If no tests were run, set received bandwidth to 0 */
                if (receiver_time > 0.0) {
                    bandwidth = (double) total_received / (double) receiver_time;
                }
                else {
                    bandwidth = 0.0;
                }
                unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
                if (test->json_output)
                    cJSON_AddItemToObject(test->json_end, "sum_received", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f sender: %b", (double) start_time, (double) receiver_time, (double) receiver_time, (int64_t) total_received, bandwidth * 8, stream_must_be_sender));
                else
                    if (test->role == 's' && stream_must_be_sender) {
                        if (test->verbose)
                            iperf_printf(test, report_receiver_not_available_summary_format, "SUM");
                    }
                    else {
                        iperf_printf(test, report_sum_bw_format, mbuf, start_time, receiver_time, ubuf, nbuf, report_receiver);
                    }
            } else {
                /* Summary sum, UDP. */
                avg_jitter /= test->num_streams;
                /* If no packets were sent, arbitrarily set loss percentage to 0. */
                if (total_packets > 0) {
                    lost_percent = 100.0 * lost_packets / total_packets;
                }
                else {
                    lost_percent = 0.0;
                }
                if (test->json_output)
                    cJSON_AddItemToObject(test->json_end, "sum", iperf_json_printf("start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f sender: %b", (double) start_time, (double) receiver_time, (double) receiver_time, (int64_t) total_sent, bandwidth * 8, (double) avg_jitter * 1000.0, (int64_t) lost_packets, (int64_t) total_packets, (double) lost_percent, stream_must_be_sender));
                else {
                    /*
                     * On the client we have both sender and receiver overall summary
                     * stats.  On the server we have only the side that was on the
                     * server.  Output whatever we have.
                     */
                    if (! (test->role == 's' && !stream_must_be_sender) ) {
                        unit_snprintf(ubuf, UNIT_LEN, (double) total_sent, 'A');
                        iperf_printf(test, report_sum_bw_udp_format, mbuf, start_time, sender_time, ubuf, nbuf, 0.0, 0, sender_total_packets, 0.0, "sender");
                    }
                    if (! (test->role == 's' && stream_must_be_sender) ) {

                        unit_snprintf(ubuf, UNIT_LEN, (double) total_received, 'A');
                        /* Compute received bandwidth. */
                        if (end_time > 0.0) {
                            bandwidth = (double) total_received / (double) receiver_time;
                        }
                        else {
                            bandwidth = 0.0;
                        }
                        unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
                        iperf_printf(test, report_sum_bw_udp_format, mbuf, start_time, receiver_time, ubuf, nbuf, avg_jitter * 1000.0, lost_packets, receiver_total_packets, lost_percent, "receiver");
                    }
                }
            }
        }

        if (test->json_output && current_mode == upper_mode) {
            cJSON_AddItemToObject(test->json_end, "cpu_utilization_percent", iperf_json_printf("host_total: %f  host_user: %f  host_system: %f  remote_total: %f  remote_user: %f  remote_system: %f", (double) test->cpu_util[0], (double) test->cpu_util[1], (double) test->cpu_util[2], (double) test->remote_cpu_util[0], (double) test->remote_cpu_util[1], (double) test->remote_cpu_util[2]));
            if (test->protocol->id == Ptcp) {
                char *snd_congestion = NULL, *rcv_congestion = NULL;
                if (stream_must_be_sender) {
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
        else {
            if (test->verbose) {
                if (stream_must_be_sender) {
                    if (test->bidirectional) {
                        iperf_printf(test, report_cpu, report_local, stream_must_be_sender?report_sender:report_receiver, test->cpu_util[0], test->cpu_util[1], test->cpu_util[2], report_remote, stream_must_be_sender?report_receiver:report_sender, test->remote_cpu_util[0], test->remote_cpu_util[1], test->remote_cpu_util[2]);
                        iperf_printf(test, report_cpu, report_local, !stream_must_be_sender?report_sender:report_receiver, test->cpu_util[0], test->cpu_util[1], test->cpu_util[2], report_remote, !stream_must_be_sender?report_receiver:report_sender, test->remote_cpu_util[0], test->remote_cpu_util[1], test->remote_cpu_util[2]);
                    } else
                        iperf_printf(test, report_cpu, report_local, stream_must_be_sender?report_sender:report_receiver, test->cpu_util[0], test->cpu_util[1], test->cpu_util[2], report_remote, stream_must_be_sender?report_receiver:report_sender, test->remote_cpu_util[0], test->remote_cpu_util[1], test->remote_cpu_util[2]);
                }
                if (test->protocol->id == Ptcp) {
                    char *snd_congestion = NULL, *rcv_congestion = NULL;
                    if (stream_must_be_sender) {
                        snd_congestion = test->congestion_used;
                        rcv_congestion = test->remote_congestion_used;
                    }
                    else {
                        snd_congestion = test->remote_congestion_used;
                        rcv_congestion = test->congestion_used;
                    }
                    if (snd_congestion) {
                        iperf_printf(test, "snd_tcp_congestion %s\n", snd_congestion);
                    }
                    if (rcv_congestion) {
                        iperf_printf(test, "rcv_tcp_congestion %s\n", rcv_congestion);
                    }
                }
            }

            /* Print server output if we're on the client and it was requested/provided */
            if (test->role == 'c' && iperf_get_test_get_server_output(test) && !test->json_output) {
                if (test->json_server_output) {
		    char *str = cJSON_Print(test->json_server_output);
                    iperf_printf(test, "\nServer JSON output:\n%s\n", str);
		    cJSON_free(str);
                    cJSON_Delete(test->json_server_output);
                    test->json_server_output = NULL;
                }
                if (test->server_output_text) {
                    iperf_printf(test, "\nServer output:\n%s\n", test->server_output_text);
                    test->server_output_text = NULL;
                }
            }
        }
    }

    /* Set real sender_has_retransmits for current side */
    if (test->mode == BIDIRECTIONAL)
        test->sender_has_retransmits = tmp_sender_has_retransmits;
}

/**************************************************************************/

/**
 * Main report-printing callback.
 * Prints results either during a test (interval report only) or 
 * after the entire test has been run (last interval report plus 
 * overall summary).
 */
void
iperf_reporter_callback(struct iperf_test *test)
{
    switch (test->state) {
        case TEST_RUNNING:
        case STREAM_RUNNING:
            /* print interval results for each stream */
            iperf_print_intermediate(test);
            break;
        case TEST_END:
        case DISPLAY_RESULTS:
            iperf_print_intermediate(test);
            iperf_print_results(test);
            break;
    } 

}

/**
 * Print the interval results for one stream.
 * This function needs to know about the overall test so it can determine the
 * context for printing headers, separators, etc.
 */
static void
print_interval_results(struct iperf_test *test, struct iperf_stream *sp, cJSON *json_interval_streams)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    char cbuf[UNIT_LEN];
    char mbuf[UNIT_LEN];
    char zbuf[] = "          ";
    double st = 0., et = 0.;
    struct iperf_time temp_time;
    struct iperf_interval_results *irp = NULL;
    double bandwidth, lost_percent;

    if (test->mode == BIDIRECTIONAL) {
        sprintf(mbuf, "[%s-%s]", sp->sender?"TX":"RX", test->role == 'c'?"C":"S");
    } else {
        mbuf[0] = '\0';
        zbuf[0] = '\0';
    }

    irp = TAILQ_LAST(&sp->result->interval_results, irlisthead); /* get last entry in linked list */
    if (irp == NULL) {
	iperf_err(test, "print_interval_results error: interval_results is NULL");
        return;
    }
    if (!test->json_output) {
	/* First stream? */
	if (sp == SLIST_FIRST(&test->streams)) {
	    /* It it's the first interval, print the header;
	    ** else if there's more than one stream, print the separator;
	    ** else nothing.
	    */
	    if (iperf_time_compare(&sp->result->start_time, &irp->interval_start_time) == 0) {
		if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
		    if (test->sender_has_retransmits == 1) {
		        if (test->bidirectional)
		            iperf_printf(test, "%s", report_bw_retrans_cwnd_header_bidir);
		        else
		            iperf_printf(test, "%s", report_bw_retrans_cwnd_header);
		    }
		    else {
	                if (test->bidirectional)
	                    iperf_printf(test, "%s", report_bw_header_bidir);
	                else
	                    iperf_printf(test, "%s", report_bw_header);
	            }
		} else {
		    if (test->mode == SENDER) {
		        iperf_printf(test, "%s", report_bw_udp_sender_header);
		    } else if (test->mode == RECEIVER){
		        iperf_printf(test, "%s", report_bw_udp_header);
		    } else {
		        /* BIDIRECTIONAL */
		        iperf_printf(test, "%s", report_bw_udp_header_bidir);
		    }
		}
	    } else if (test->num_streams > 1)
		iperf_printf(test, "%s", report_bw_separator);
	}
    }

    unit_snprintf(ubuf, UNIT_LEN, (double) (irp->bytes_transferred), 'A');
    if (irp->interval_duration > 0.0) {
	bandwidth = (double) irp->bytes_transferred / (double) irp->interval_duration;
    }
    else {
	bandwidth = 0.0;
    }
    unit_snprintf(nbuf, UNIT_LEN, bandwidth, test->settings->unit_format);
    
    iperf_time_diff(&sp->result->start_time, &irp->interval_start_time, &temp_time);
    st = iperf_time_in_secs(&temp_time);
    iperf_time_diff(&sp->result->start_time, &irp->interval_end_time, &temp_time);
    et = iperf_time_in_secs(&temp_time);
    
    if (test->protocol->id == Ptcp || test->protocol->id == Psctp) {
	if (test->sender_has_retransmits == 1 && sp->sender) {
	    /* Interval, TCP with retransmits. */
	    if (test->json_output)
		cJSON_AddItemToArray(json_interval_streams, iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  retransmits: %d  snd_cwnd:  %d  rtt:  %d  rttvar: %d  pmtu: %d  omitted: %b sender: %b", (int64_t) sp->socket, (double) st, (double) et, (double) irp->interval_duration, (int64_t) irp->bytes_transferred, bandwidth * 8, (int64_t) irp->interval_retrans, (int64_t) irp->snd_cwnd, (int64_t) irp->rtt, (int64_t) irp->rttvar, (int64_t) irp->pmtu, irp->omitted, sp->sender));
	    else {
		unit_snprintf(cbuf, UNIT_LEN, irp->snd_cwnd, 'A');
		iperf_printf(test, report_bw_retrans_cwnd_format, sp->socket, mbuf, st, et, ubuf, nbuf, irp->interval_retrans, cbuf, irp->omitted?report_omitted:"");
	    }
	} else {
	    /* Interval, TCP without retransmits. */
	    if (test->json_output)
		cJSON_AddItemToArray(json_interval_streams, iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  omitted: %b sender: %b", (int64_t) sp->socket, (double) st, (double) et, (double) irp->interval_duration, (int64_t) irp->bytes_transferred, bandwidth * 8, irp->omitted, sp->sender));
	    else
		iperf_printf(test, report_bw_format, sp->socket, mbuf, st, et, ubuf, nbuf, irp->omitted?report_omitted:"");
	}
    } else {
	/* Interval, UDP. */
	if (sp->sender) {
	    if (test->json_output)
		cJSON_AddItemToArray(json_interval_streams, iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  packets: %d  omitted: %b sender: %b", (int64_t) sp->socket, (double) st, (double) et, (double) irp->interval_duration, (int64_t) irp->bytes_transferred, bandwidth * 8, (int64_t) irp->interval_packet_count, irp->omitted, sp->sender));
	    else
		iperf_printf(test, report_bw_udp_sender_format, sp->socket, mbuf, st, et, ubuf, nbuf, zbuf, irp->interval_packet_count, irp->omitted?report_omitted:"");
	} else {
	    if (irp->interval_packet_count > 0) {
		lost_percent = 100.0 * irp->interval_cnt_error / irp->interval_packet_count;
	    }
	    else {
		lost_percent = 0.0;
	    }
	    if (test->json_output)
		cJSON_AddItemToArray(json_interval_streams, iperf_json_printf("socket: %d  start: %f  end: %f  seconds: %f  bytes: %d  bits_per_second: %f  jitter_ms: %f  lost_packets: %d  packets: %d  lost_percent: %f  omitted: %b sender: %b", (int64_t) sp->socket, (double) st, (double) et, (double) irp->interval_duration, (int64_t) irp->bytes_transferred, bandwidth * 8, (double) irp->jitter * 1000.0, (int64_t) irp->interval_cnt_error, (int64_t) irp->interval_packet_count, (double) lost_percent, irp->omitted, sp->sender));
	    else
		iperf_printf(test, report_bw_udp_format, sp->socket, mbuf, st, et, ubuf, nbuf, irp->jitter * 1000.0, irp->interval_cnt_error, irp->interval_packet_count, lost_percent, irp->omitted?report_omitted:"");
	}
    }

    if (test->logfile || test->forceflush)
        iflush(test);
}

/**************************************************************************/
void
iperf_free_stream(struct iperf_stream *sp)
{
    struct iperf_interval_results *irp, *nirp;

    /* XXX: need to free interval list too! */
    munmap(sp->buffer, sp->test->settings->blksize);
    close(sp->buffer_fd);
    if (sp->diskfile_fd >= 0)
	close(sp->diskfile_fd);
    for (irp = TAILQ_FIRST(&sp->result->interval_results); irp != NULL; irp = nirp) {
        nirp = TAILQ_NEXT(irp, irlistentries);
        free(irp);
    }
    free(sp->result);
    if (sp->send_timer != NULL)
	tmr_cancel(sp->send_timer);
    free(sp);
}

/**************************************************************************/
struct iperf_stream *
iperf_new_stream(struct iperf_test *test, int s, int sender)
{
    struct iperf_stream *sp;
    int ret = 0;

    char template[1024];
    if (test->tmp_template) {
        snprintf(template, sizeof(template) / sizeof(char), "%s", test->tmp_template);
    } else {
        //find the system temporary dir *unix, windows, cygwin support
        char* tempdir = getenv("TMPDIR");
        if (tempdir == 0){
            tempdir = getenv("TEMP");
        }
        if (tempdir == 0){
            tempdir = getenv("TMP");
        }
        if (tempdir == 0){
            tempdir = "/tmp";
        }
        snprintf(template, sizeof(template) / sizeof(char), "%s/iperf3.XXXXXX", tempdir);
    }

    sp = (struct iperf_stream *) malloc(sizeof(struct iperf_stream));
    if (!sp) {
        i_errno = IECREATESTREAM;
        return NULL;
    }

    memset(sp, 0, sizeof(struct iperf_stream));

    sp->sender = sender;
    sp->test = test;
    sp->settings = test->settings;
    sp->result = (struct iperf_stream_result *) malloc(sizeof(struct iperf_stream_result));
    if (!sp->result) {
        free(sp);
        i_errno = IECREATESTREAM;
        return NULL;
    }

    memset(sp->result, 0, sizeof(struct iperf_stream_result));
    TAILQ_INIT(&sp->result->interval_results);
    
    /* Create and randomize the buffer */
    sp->buffer_fd = mkstemp(template);
    if (sp->buffer_fd == -1) {
        i_errno = IECREATESTREAM;
        free(sp->result);
        free(sp);
        return NULL;
    }
    if (unlink(template) < 0) {
        i_errno = IECREATESTREAM;
        free(sp->result);
        free(sp);
        return NULL;
    }
    if (ftruncate(sp->buffer_fd, test->settings->blksize) < 0) {
        i_errno = IECREATESTREAM;
        free(sp->result);
        free(sp);
        return NULL;
    }
    sp->buffer = (char *) mmap(NULL, test->settings->blksize, PROT_READ|PROT_WRITE, MAP_PRIVATE, sp->buffer_fd, 0);
    if (sp->buffer == MAP_FAILED) {
        i_errno = IECREATESTREAM;
        free(sp->result);
        free(sp);
        return NULL;
    }

    /* Set socket */
    sp->socket = s;

    sp->snd = test->protocol->send;
    sp->rcv = test->protocol->recv;

    if (test->diskfile_name != (char*) 0) {
	sp->diskfile_fd = open(test->diskfile_name, sender ? O_RDONLY : (O_WRONLY|O_CREAT|O_TRUNC), S_IRUSR|S_IWUSR);
	if (sp->diskfile_fd == -1) {
	    i_errno = IEFILE;
            munmap(sp->buffer, sp->test->settings->blksize);
            free(sp->result);
            free(sp);
	    return NULL;
	}
        sp->snd2 = sp->snd;
	sp->snd = diskfile_send;
	sp->rcv2 = sp->rcv;
	sp->rcv = diskfile_recv;
    } else
        sp->diskfile_fd = -1;

    /* Initialize stream */
    if (test->repeating_payload)
        fill_with_repeating_pattern(sp->buffer, test->settings->blksize);
    else
        ret = readentropy(sp->buffer, test->settings->blksize);

    if ((ret < 0) || (iperf_init_stream(sp, test) < 0)) {
        close(sp->buffer_fd);
        munmap(sp->buffer, sp->test->settings->blksize);
        free(sp->result);
        free(sp);
        return NULL;
    }
    iperf_add_stream(test, sp);

    return sp;
}

/**************************************************************************/
int
iperf_init_stream(struct iperf_stream *sp, struct iperf_test *test)
{
    socklen_t len;
    int opt;

    len = sizeof(struct sockaddr_storage);
    if (getsockname(sp->socket, (struct sockaddr *) &sp->local_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return -1;
    }
    len = sizeof(struct sockaddr_storage);
    if (getpeername(sp->socket, (struct sockaddr *) &sp->remote_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return -1;
    }

    /* Set IP TOS */
    if ((opt = test->settings->tos)) {
        if (getsockdomain(sp->socket) == AF_INET6) {
#ifdef IPV6_TCLASS
            if (setsockopt(sp->socket, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETCOS;
                return -1;
            }
#else
            i_errno = IESETCOS;
            return -1;
#endif
        } else {
            if (setsockopt(sp->socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETTOS;
                return -1;
            }
        }
    }

    return 0;
}

/**************************************************************************/
void
iperf_add_stream(struct iperf_test *test, struct iperf_stream *sp)
{
    int i;
    struct iperf_stream *n, *prev;

    if (SLIST_EMPTY(&test->streams)) {
        SLIST_INSERT_HEAD(&test->streams, sp, streams);
        sp->id = 1;
    } else {
        // for (n = test->streams, i = 2; n->next; n = n->next, ++i);
        i = 2;
        SLIST_FOREACH(n, &test->streams, streams) {
            prev = n;
            ++i;
        }
        SLIST_INSERT_AFTER(prev, sp, streams);
        sp->id = i;
    }
}

/* This pair of routines gets inserted into the snd/rcv function pointers
** when there's a -F flag. They handle the file stuff and call the real
** snd/rcv functions, which have been saved in snd2/rcv2.
**
** The advantage of doing it this way is that in the much more common
** case of no -F flag, there is zero extra overhead.
*/

static int
diskfile_send(struct iperf_stream *sp)
{
    int r;
    static int rtot;

    /* if needed, read enough data from the disk to fill up the buffer */
    if (sp->diskfile_left < sp->test->settings->blksize && !sp->test->done) {
	r = read(sp->diskfile_fd, sp->buffer, sp->test->settings->blksize -
		 sp->diskfile_left);
	rtot += r;
	if (sp->test->debug) {
	    printf("read %d bytes from file, %d total\n", r, rtot);
	    if (r != sp->test->settings->blksize - sp->diskfile_left)
		printf("possible eof\n");
	}
	/* If there's no data left in the file or in the buffer, we're done */
	if (r == 0 && sp->diskfile_left == 0) {
	    sp->test->done = 1;
	    if (sp->test->debug)
		printf("done\n");
	}
    }

    r = sp->snd2(sp);
    if (r < 0) {
	return r;
    }
    /*
     * Compute how much data is in the buffer but didn't get sent.
     * If there are bytes that got left behind, slide them to the
     * front of the buffer so they can hopefully go out on the next
     * pass.
     */
    sp->diskfile_left = sp->test->settings->blksize - r;
    if (sp->diskfile_left && sp->diskfile_left < sp->test->settings->blksize) {
	memcpy(sp->buffer,
	       sp->buffer + (sp->test->settings->blksize - sp->diskfile_left),
	       sp->diskfile_left);
	if (sp->test->debug)
	    printf("Shifting %d bytes by %d\n", sp->diskfile_left, (sp->test->settings->blksize - sp->diskfile_left));
    }
    return r;
}

static int
diskfile_recv(struct iperf_stream *sp)
{
    int r;

    r = sp->rcv2(sp);
    if (r > 0) {
	(void) write(sp->diskfile_fd, sp->buffer, r);
	(void) fsync(sp->diskfile_fd);
    }
    return r;
}


void
iperf_catch_sigend(void (*handler)(int))
{
#ifdef SIGINT
    signal(SIGINT, handler);
#endif
#ifdef SIGTERM
    signal(SIGTERM, handler);
#endif
#ifdef SIGHUP
    signal(SIGHUP, handler);
#endif
}

/**
 * Called as a result of getting a signal.
 * Depending on the current state of the test (and the role of this
 * process) compute and report one more set of ending statistics
 * before cleaning up and exiting.
 */
void
iperf_got_sigend(struct iperf_test *test)
{
    /*
     * If we're the client, or if we're a server and running a test,
     * then dump out the accumulated stats so far.
     */
    if (test->role == 'c' ||
      (test->role == 's' && test->state == TEST_RUNNING)) {

	test->done = 1;
	cpu_util(test->cpu_util);
	test->stats_callback(test);
	test->state = DISPLAY_RESULTS; /* change local state only */
	if (test->on_test_finish)
	    test->on_test_finish(test);
	test->reporter_callback(test);
    }

    if (test->ctrl_sck >= 0) {
	test->state = (test->role == 'c') ? CLIENT_TERMINATE : SERVER_TERMINATE;
	(void) Nwrite(test->ctrl_sck, (char*) &test->state, sizeof(signed char), Ptcp);
    }
    i_errno = (test->role == 'c') ? IECLIENTTERM : IESERVERTERM;
    iperf_errexit(test, "interrupt - %s", iperf_strerror(i_errno));
}

/* Try to write a PID file if requested, return -1 on an error. */
int
iperf_create_pidfile(struct iperf_test *test)
{
    if (test->pidfile) {
	int fd;
	char buf[8];

	/* See if the file already exists and we can read it. */
	fd = open(test->pidfile, O_RDONLY, 0);
	if (fd >= 0) {
	    if (read(fd, buf, sizeof(buf) - 1) >= 0) {

		/* We read some bytes, see if they correspond to a valid PID */
		pid_t pid;
		pid = atoi(buf);
		if (pid > 0) {

		    /* See if the process exists. */
		    if (kill(pid, 0) == 0) {
			/*
			 * Make sure not to try to delete existing PID file by
			 * scribbling over the pathname we'd use to refer to it.
			 * Then exit with an error.
			 */
			free(test->pidfile);
			test->pidfile = NULL;
			iperf_errexit(test, "Another instance of iperf3 appears to be running");
		    }
		}
	    }
	}
	
	/*
	 * File didn't exist, we couldn't read it, or it didn't correspond to 
	 * a running process.  Try to create it. 
	 */
	fd = open(test->pidfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd < 0) {
	    return -1;
	}
	snprintf(buf, sizeof(buf), "%d", getpid()); /* no trailing newline */
	if (write(fd, buf, strlen(buf) + 1) < 0) {
	    return -1;
	}
	if (close(fd) < 0) {
	    return -1;
	};
    }
    return 0;
}

/* Get rid of a PID file, return -1 on error. */
int
iperf_delete_pidfile(struct iperf_test *test)
{
    if (test->pidfile) {
	if (unlink(test->pidfile) < 0) {
	    return -1;
	}
    }
    return 0;
}

int
iperf_json_start(struct iperf_test *test)
{
    test->json_top = cJSON_CreateObject();
    if (test->json_top == NULL)
        return -1;
    test->json_start = cJSON_CreateObject();
    if (test->json_start == NULL)
        return -1;
    cJSON_AddItemToObject(test->json_top, "start", test->json_start);
    test->json_connected = cJSON_CreateArray();
    if (test->json_connected == NULL)
        return -1;
    cJSON_AddItemToObject(test->json_start, "connected", test->json_connected);
    test->json_intervals = cJSON_CreateArray();
    if (test->json_intervals == NULL)
        return -1;
    cJSON_AddItemToObject(test->json_top, "intervals", test->json_intervals);
    test->json_end = cJSON_CreateObject();
    if (test->json_end == NULL)
        return -1;
    cJSON_AddItemToObject(test->json_top, "end", test->json_end);
    return 0;
}

int
iperf_json_finish(struct iperf_test *test)
{
    if (test->title)
	cJSON_AddStringToObject(test->json_top, "title", test->title);
    if (test->extra_data)
	cJSON_AddStringToObject(test->json_top, "extra_data", test->extra_data);
    /* Include server output */
    if (test->json_server_output) {
	cJSON_AddItemToObject(test->json_top, "server_output_json", test->json_server_output);
    }
    if (test->server_output_text) {
	cJSON_AddStringToObject(test->json_top, "server_output_text", test->server_output_text);
    }
    test->json_output_string = cJSON_Print(test->json_top);
    if (test->json_output_string == NULL)
        return -1;
    fprintf(test->outfile, "%s\n", test->json_output_string);
    iflush(test);
    cJSON_free(test->json_output_string);
    test->json_output_string = NULL;
    cJSON_Delete(test->json_top);
    test->json_top = test->json_start = test->json_connected = test->json_intervals = test->json_server_output = test->json_end = NULL;
    return 0;
}


/* CPU affinity stuff - Linux, FreeBSD, and Windows only. */

int
iperf_setaffinity(struct iperf_test *test, int affinity)
{
#if defined(HAVE_SCHED_SETAFFINITY)
    cpu_set_t cpu_set;

    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
	i_errno = IEAFFINITY;
        return -1;
    }
    return 0;
#elif defined(HAVE_CPUSET_SETAFFINITY)
    cpuset_t cpumask;

    if(cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                          sizeof(cpuset_t), &test->cpumask) != 0) {
        i_errno = IEAFFINITY;
        return -1;
    }

    CPU_ZERO(&cpumask);
    CPU_SET(affinity, &cpumask);

    if(cpuset_setaffinity(CPU_LEVEL_WHICH,CPU_WHICH_PID, -1,
                          sizeof(cpuset_t), &cpumask) != 0) {
        i_errno = IEAFFINITY;
        return -1;
    }
    return 0;
#elif defined(HAVE_SETPROCESSAFFINITYMASK)
	HANDLE process = GetCurrentProcess();
	DWORD_PTR processAffinityMask = 1 << affinity;

	if (SetProcessAffinityMask(process, processAffinityMask) == 0) {
		i_errno = IEAFFINITY;
		return -1;
	}
	return 0;
#else /* neither HAVE_SCHED_SETAFFINITY nor HAVE_CPUSET_SETAFFINITY nor HAVE_SETPROCESSAFFINITYMASK */
    i_errno = IEAFFINITY;
    return -1;
#endif /* neither HAVE_SCHED_SETAFFINITY nor HAVE_CPUSET_SETAFFINITY nor HAVE_SETPROCESSAFFINITYMASK */
}

int
iperf_clearaffinity(struct iperf_test *test)
{
#if defined(HAVE_SCHED_SETAFFINITY)
    cpu_set_t cpu_set;
    int i;

    CPU_ZERO(&cpu_set);
    for (i = 0; i < CPU_SETSIZE; ++i)
	CPU_SET(i, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
	i_errno = IEAFFINITY;
        return -1;
    }
    return 0;
#elif defined(HAVE_CPUSET_SETAFFINITY)
    if(cpuset_setaffinity(CPU_LEVEL_WHICH,CPU_WHICH_PID, -1,
                          sizeof(cpuset_t), &test->cpumask) != 0) {
        i_errno = IEAFFINITY;
        return -1;
    }
    return 0;
#elif defined(HAVE_SETPROCESSAFFINITYMASK)
	HANDLE process = GetCurrentProcess();
	DWORD_PTR processAffinityMask;
	DWORD_PTR lpSystemAffinityMask;

	if (GetProcessAffinityMask(process, &processAffinityMask, &lpSystemAffinityMask) == 0
			|| SetProcessAffinityMask(process, lpSystemAffinityMask) == 0) {
		i_errno = IEAFFINITY;
		return -1;
	}
	return 0;
#else /* neither HAVE_SCHED_SETAFFINITY nor HAVE_CPUSET_SETAFFINITY nor HAVE_SETPROCESSAFFINITYMASK */
    i_errno = IEAFFINITY;
    return -1;
#endif /* neither HAVE_SCHED_SETAFFINITY nor HAVE_CPUSET_SETAFFINITY nor HAVE_SETPROCESSAFFINITYMASK */
}

int
iperf_printf(struct iperf_test *test, const char* format, ...)
{
    va_list argp;
    int r = -1;

    /*
     * There are roughly two use cases here.  If we're the client,
     * want to print stuff directly to the output stream.
     * If we're the sender we might need to buffer up output to send
     * to the client.
     *
     * This doesn't make a whole lot of difference except there are
     * some chunks of output on the client (on particular the whole
     * of the server output with --get-server-output) that could
     * easily exceed the size of the line buffer, but which don't need
     * to be buffered up anyway.
     */
    if (test->role == 'c') {
	if (test->title)
	    fprintf(test->outfile, "%s:  ", test->title);
	va_start(argp, format);
	r = vfprintf(test->outfile, format, argp);
	va_end(argp);
    }
    else if (test->role == 's') {
	char linebuffer[1024];
	va_start(argp, format);
	r = vsnprintf(linebuffer, sizeof(linebuffer), format, argp);
	va_end(argp);
	fprintf(test->outfile, "%s", linebuffer);

	if (test->role == 's' && iperf_get_test_get_server_output(test)) {
	    struct iperf_textline *l = (struct iperf_textline *) malloc(sizeof(struct iperf_textline));
	    l->line = strdup(linebuffer);
	    TAILQ_INSERT_TAIL(&(test->server_output_list), l, textlineentries);
	}
    }
    return r;
}

int
iflush(struct iperf_test *test)
{
    return fflush(test->outfile);
}
