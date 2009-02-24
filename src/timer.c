#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>

#include "timer.h"

int
timer_expired(struct timer *tp)
{
    struct timeval now;
    if(gettimeofday(&now, NULL) < 0) {
        perror("gettimeofday");
        return -1;
    }

    return tp->end.tv_sec <= now.tv_sec;
}

struct timer *
new_timer(time_t sec, suseconds_t usec)
{
    struct timer *tp;
    tp = (struct timer *) malloc(sizeof(struct timer));

    if(gettimeofday(&tp->begin, NULL) < 0) {
        perror("gettimeofday");
        return NULL;
    }

    memcpy(&tp->end, &tp->begin, sizeof(struct timer));
    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;

    return tp;
}

void
free_timer(struct timer *tp)
{
    free(tp);
}

int
delay(int64_t ns)
{
    struct timespec req, rem;

    req.tv_sec = 0;

    while(ns >= 1000000000L)
    {
        ns -= 1000000000L;
        req.tv_sec += 1;
    }

    req.tv_nsec = ns;

    while(nanosleep(&req, &rem) == -1 )
        if (EINTR == errno)
            memcpy(&req, &rem, sizeof rem);
        else
            return -1;

    return 0;
}

