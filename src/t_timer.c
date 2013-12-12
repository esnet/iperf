/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "timer.h"


static int flag;


static void
timer_proc( TimerClientData client_data, struct timeval* nowP )
{
    flag = 1;
}


int 
main(int argc, char **argv)
{
    Timer *tp;

    flag = 0;
    tp = tmr_create((struct timeval*) 0, timer_proc, JunkClientData, 3000000, 0);
    if (!tp)
    {
	printf("failed to create timer\n");
	exit(-1);
    }

    sleep(2);

    tmr_run((struct timeval*) 0);
    if (flag)
    {
	printf("timer should not have expired\n");
	exit(-1);
    }
    sleep(1);

    tmr_run((struct timeval*) 0);
    if (!flag)
    {
	printf("timer should have expired\n");
	exit(-2);
    }

    tmr_destroy();
    exit(0);
}
