/*
 * Copyright (c) 2009-2014, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
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

#include "iperf.h"
#include "iperf_api.h"
#include "units.h"
#include "locale.h"
#include "net.h"


static int run(struct iperf_test *test);


/**************************************************************************/
int
main(int argc, char **argv)
{
    struct iperf_test *test;

    // XXX: Setting the process affinity requires root on most systems.
    //      Is this a feature we really need?
#ifdef TEST_PROC_AFFINITY
    /* didnt seem to work.... */
    /*
     * increasing the priority of the process to minimise packet generation
     * delay
     */
    int rc = setpriority(PRIO_PROCESS, 0, -15);

    if (rc < 0) {
        perror("setpriority:");
        fprintf(stderr, "setting priority to valid level\n");
        rc = setpriority(PRIO_PROCESS, 0, 0);
    }
    
    /* setting the affinity of the process  */
    cpu_set_t cpu_set;
    int affinity = -1;
    int ncores = 1;

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
    if (!test)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test);	/* sets defaults */

    /* This main program doesn't use SIGALRM, so the iperf API may use it. */
    iperf_set_test_may_use_sigalrm(test, 1);

    if (iperf_parse_arguments(test, argc, argv) < 0) {
        iperf_err(test, "parameter error - %s", iperf_strerror(i_errno));
        fprintf(stderr, "\n");
        usage_long();
        exit(1);
    }

    if (run(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));

    iperf_free_test(test);

    return 0;
}

/**************************************************************************/
static int
run(struct iperf_test *test)
{
    int consecutive_errors;

    switch (test->role) {
        case 's':
	    if (test->daemon) {
		int rc = daemon(0, 0);
		if (rc < 0) {
		    i_errno = IEDAEMON;
		    iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
		}
	    }
	    consecutive_errors = 0;
	    if (iperf_create_pidfile(test) < 0) {
		i_errno = IEPIDFILE;
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    }
            for (;;) {
                if (iperf_run_server(test) < 0) {
		    iperf_err(test, "error - %s", iperf_strerror(i_errno));
                    fprintf(stderr, "\n");
		    ++consecutive_errors;
		    if (consecutive_errors >= 5) {
		        fprintf(stderr, "too many errors, exiting\n");
			break;
		    }
                } else
		    consecutive_errors = 0;
                iperf_reset_test(test);
            }
	    iperf_delete_pidfile(test);
            break;
        case 'c':
            if (iperf_run_client(test) < 0)
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
            break;
        default:
            usage();
            break;
    }

    return 0;
}
