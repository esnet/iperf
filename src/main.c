
/*
 * Copyright (c) 2004, The Regents of the University of California, through
 * Lawrence Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 */

/*
 * TO DO list:
 *    restructure code pull out main.c
 *    cleanup/fix/test UDP mode
 #    IPV6
 *    add verbose and debug options
 *    lots more testing
 *    see issue tracker for other wish list items
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <netinet/tcp.h>


#include "iperf_api.h"
#include "units.h"
#include "locale.h"


/**************************************************************************/

static struct option longopts[] =
{
    {"client", required_argument, NULL, 'c'},
    {"server", no_argument, NULL, 's'},
    {"time", required_argument, NULL, 't'},
    {"port", required_argument, NULL, 'p'},
    {"parallel", required_argument, NULL, 'P'},
    {"udp", no_argument, NULL, 'u'},
    {"tcpInfo", no_argument, NULL, 'T'},
    {"bandwidth", required_argument, NULL, 'b'},
    {"length", required_argument, NULL, 'l'},
    {"window", required_argument, NULL, 'w'},
    {"interval", required_argument, NULL, 'i'},
    {"bytes", required_argument, NULL, 'n'},
    {"NoDelay", no_argument, NULL, 'N'},
    {"Print-mss", no_argument, NULL, 'm'},
    {"Set-mss", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

/**************************************************************************/

int
main(int argc, char **argv)
{

    char      ch, role;
    struct iperf_test *test;
    int       port = PORT;

#ifdef TEST_PROC_AFFINITY
    /* didnt seem to work.... */
    /*
     * increasing the priority of the process to minimise packet generation
     * delay
     */
    int       rc = setpriority(PRIO_PROCESS, 0, -15);

    if (rc < 0)
    {
	perror("setpriority:");
	printf("setting priority to valid level\n");
	rc = setpriority(PRIO_PROCESS, 0, 0);
    }
    /* setting the affinity of the process  */
    cpu_set_t cpu_set;
    int       affinity = -1;
    int       ncores = 1;

    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
    if (errno)
	perror("couldn't get affinity:");

    if ((ncores = sysconf(_SC_NPROCESSORS_CONF)) <= 0)
	err("sysconf: couldn't get _SC_NPROCESSORS_CONF");

    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0)
	err("couldn't change CPU affinity");
#endif

    test = iperf_new_test();
    iperf_defaults(test);	/* sets defaults */

    while ((ch = getopt_long(argc, argv, "c:p:st:uP:b:l:w:i:n:mNThM:f:", longopts, NULL)) != -1)
    {
	switch (ch)
	{
	case 'c':
	    test->role = 'c';
	    role = test->role;
	    test->server_hostname = (char *) malloc(strlen(optarg));
	    strncpy(test->server_hostname, optarg, strlen(optarg));
	    break;
	case 'p':
	    test->server_port = atoi(optarg);
	    port = test->server_port;
	    break;
	case 's':
	    test->role = 's';
	    role = test->role;
	    break;
	case 't':
	    test->duration = atoi(optarg);
	    break;
	case 'u':
	    test->protocol = Pudp;
	    test->default_settings->blksize = DEFAULT_UDP_BLKSIZE;
	    test->new_stream = iperf_new_udp_stream;
	    break;
	case 'P':
	    test->num_streams = atoi(optarg);
	    break;
	case 'b':
	    test->default_settings->rate = unit_atof(optarg);
	    break;
	case 'l':
	    test->default_settings->blksize = unit_atoi(optarg);
	    printf("%d is the blksize\n", test->default_settings->blksize);
	    break;
	case 'w':
	    test->default_settings->socket_bufsize = unit_atof(optarg);
	    break;
	case 'i':
	    test->stats_interval = atoi(optarg);
	    test->reporter_interval = atoi(optarg);
	    break;
	case 'n':
	    test->default_settings->bytes = unit_atoi(optarg);
	    printf("total bytes to be transferred = %llu\n", test->default_settings->bytes);
	    break;
	case 'm':
	    test->print_mss = 1;
	    break;
	case 'N':
	    test->no_delay = 1;
	    break;
	case 'M':
	    test->default_settings->mss = atoi(optarg);
	    break;
	case 'f':
	    test->default_settings->unit_format = *optarg;
	    break;
	case 'T':
	    test->tcp_info = 1;
	    break;
	case 'h':
	default:
	    fprintf(stderr, usage_long1);
	    fprintf(stderr, usage_long2);
	    exit(1);
	}
    }


    //printf("in main: calling iperf_init_test \n");
    printf("Connection to port %d on host %s \n", test->server_port, test->server_hostname);
    iperf_init_test(test);

    if (test->role == 'c')	/* if client, send params to server */
    {
	exchange_parameters(test);
	test->streams->settings->state = STREAM_BEGIN;
    }
    //printf("in main: calling iperf_run \n");
    iperf_run(test);
    iperf_free_test(test);

    printf("\niperf Done.\n");
    exit(0);
}
