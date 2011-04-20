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
#include <errno.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>

#include "timer.h"
#include "iperf_error.h"

double
timeval_to_double(struct timeval * tv)
{
    double d;

    d = tv->tv_sec + tv->tv_usec / 1000000;

    return d;
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
timer_expired(struct timer * tp)
{
    if (tp == NULL)
        return 0;

    struct timeval now;
    int64_t end = 0, current = 0;

    gettimeofday(&now, NULL);

    end += tp->end.tv_sec * 1000000;
    end += tp->end.tv_usec;

    current += now.tv_sec * 1000000;
    current += now.tv_usec;

    return current > end;
}

int
update_timer(struct timer * tp, time_t sec, suseconds_t usec)
{
    if (gettimeofday(&tp->begin, NULL) < 0) {
        i_errno = IEUPDATETIMER;
        return (-1);
    }

    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;
    return (0);
}

struct timer *
new_timer(time_t sec, suseconds_t usec)
{
    struct timer *tp = NULL;
    tp = (struct timer *) calloc(1, sizeof(struct timer));
    if (tp == NULL) {
        i_errno = IENEWTIMER;
        return (NULL);
    }

    if (gettimeofday(&tp->begin, NULL) < 0) {
        i_errno = IENEWTIMER;
        return (NULL);
    }

    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;

    return tp;
}

void
free_timer(struct timer * tp)
{
    free(tp);
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

int64_t
timer_remaining(struct timer * tp)
{
    struct timeval now;
    long int  end_time = 0, current_time = 0, diff = 0;

    gettimeofday(&now, NULL);

    end_time += tp->end.tv_sec * 1000000;
    end_time += tp->end.tv_usec;

    current_time += now.tv_sec * 1000000;
    current_time += now.tv_usec;

    diff = end_time - current_time;
    if (diff > 0)
        return diff;
    else
        return 0;
}

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

