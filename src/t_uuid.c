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
#include <string.h>
#include <sys/time.h>

#include "iperf_util.h"

int
main(int argc, char **argv)
{
    char     cookie[37];
    make_cookie(cookie);
    printf("cookie: '%s'\n", cookie);
    if (strlen(cookie) != 36)
    {
	printf("Not 36 characters long!\n");
	exit(-1);
    }
    exit(0);
}
