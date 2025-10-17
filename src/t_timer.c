/*
 * iperf, Copyright (c) 2014, The Regents of the University of
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
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "timer.h"
#include "iperf_time.h"


static int flag;


static void
timer_proc( TimerClientData client_data, struct iperf_time* nowP )
{
    flag = 1;
}


int
main(int argc, char **argv)
{
    Timer *tp;

    flag = 0;
    tp = tmr_create(NULL, timer_proc, JunkClientData, 3000000, 0);
    if (!tp)
    {
	printf("failed to create timer\n");
	exit(-1);
    }

    sleep(2);

    tmr_run(NULL);
    if (flag)
    {
	printf("timer should not have expired\n");
	exit(-1);
    }
    sleep(1);

    tmr_run(NULL);
    if (!flag)
    {
	printf("timer should have expired\n");
	exit(-2);
    }

    tmr_destroy();
    exit(0);
}
