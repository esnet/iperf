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

int 
main(int argc, char **argv)
{
    struct timer *tp;
    tp = new_timer(3, 0);

    sleep(2);

    if (tp->expired(tp))
    {
	printf("timer should not have expired\n");
	exit(-1);
    }
    sleep(1);

    if (!tp->expired(tp))
    {
	printf("timer should have expired\n");
	exit(-2);
    }
    exit(0);
}
