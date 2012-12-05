/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

/* iperf_util.c
 *
 * Iperf utility functions
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "config.h"

/* make_cookie
 *
 * Generate and return a cookie string
 *
 * Iperf uses this function to create test "cookies" which
 * server as unique test identifiers. These cookies are also
 * used for the authentication of stream connections.
 */

void
make_cookie(char *cookie)
{
    static int randomized = 0;
    char hostname[500];
    struct timeval tv;
    char temp[1000];

    if ( ! randomized )
        srandom((int) time(0) ^ getpid());

    /* Generate a string based on hostname, time, randomness, and filler. */
    (void) gethostname(hostname, sizeof(hostname));
    (void) gettimeofday(&tv, 0);
    (void) snprintf(temp, sizeof(temp), "%s.%ld.%06ld.%08lx%08lx.%s", hostname, (unsigned long int) tv.tv_sec, (unsigned long int) tv.tv_usec, (unsigned long int) random(), (unsigned long int) random(), "1234567890123456789012345678901234567890");

    /* Now truncate it to 36 bytes and terminate. */
    memcpy(cookie, temp, 36);
    cookie[36] = '\0';
}


/* is_closed
 *
 * Test if the file descriptor fd is closed.
 * 
 * Iperf uses this function to test whether a TCP stream socket
 * is closed, because accepting and denying an invalid connection
 * in iperf_tcp_accept is not considered an error.
 */

int
is_closed(int fd)
{
    struct timeval tv;
    fd_set readset;

    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (select(fd+1, &readset, NULL, NULL, &tv) < 0) {
        if (errno == EBADF)
            return (1);
    }
    return (0);
}


double
timeval_to_double(struct timeval * tv)
{
    double d;

    d = tv->tv_sec + tv->tv_usec / 1000000;

    return d;
}

int
timeval_equals(struct timeval * tv0, struct timeval * tv1)
{
    if ( tv0->tv_sec == tv1->tv_sec && tv0->tv_usec == tv1->tv_usec )
	return 1;
    else
	return 0;
}

double
timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    double time1, time2;
    
    time1 = tv0->tv_sec + (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec + (tv1->tv_usec / 1000000.0);

    time1 = time1 - time2;
    if (time1 < 0)
        time1 = -time1;
    return (time1);
}


int
delay(int64_t ns)
{
    struct timespec req, rem;

    req.tv_sec = 0;

    while (ns >= 1000000000L) {
        ns -= 1000000000L;
        req.tv_sec += 1;
    }

    req.tv_nsec = ns;

    while (nanosleep(&req, &rem) == -1)
        if (EINTR == errno)
            memcpy(&req, &rem, sizeof rem);
        else
            return -1;
    return 0;
}

# ifdef DELAY_SELECT_METHOD
int
delay(int us)
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = us;
    (void) select(1, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv);
    return (1);
}
#endif


void
cpu_util(double *pcpu)
{
    static struct timeval last;
    static clock_t clast;
    struct timeval temp;
    clock_t ctemp;
    double timediff;

    if (pcpu == NULL) {
        gettimeofday(&last, NULL);
        clast = clock();
        return;
    }

    gettimeofday(&temp, NULL);
    ctemp = clock();

    timediff = ((temp.tv_sec * 1000000.0 + temp.tv_usec) -
            (last.tv_sec * 1000000.0 + last.tv_usec));

    *pcpu = ((ctemp - clast) / timediff) * 100;
}
