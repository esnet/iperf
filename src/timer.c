#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>

#include "timer.h"



double
timeval_to_double(struct timeval * tv)
{
    double    d;

    d = tv->tv_sec + tv->tv_usec / 1000000;

    return d;
}

double
timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    //return timeval_to_double(tv1) - timeval_to_double(tv0);
    return (tv1->tv_sec - tv0->tv_sec) + abs(tv1->tv_usec - tv0->tv_usec) / 1000000.0;
}

int
timer_expired(struct timer * tp)
{
    /* for timer with zero time */
    if (tp->end.tv_sec == tp->begin.tv_sec && tp->end.tv_usec == tp->begin.tv_usec)
    {
	//printf(" timer_expired: begining and end times are equal \n");
	return 0;
    }

    struct timeval now;
    int64_t   end = 0, current = 0, diff = 0;

    //printf("checking if timer has expired \n");
    if (gettimeofday(&now, NULL) < 0)
    {
	perror("gettimeofday");
	return -1;
    }
    end += tp->end.tv_sec * 1000000;
    end += tp->end.tv_usec;

    current += now.tv_sec * 1000000;
    current += now.tv_usec;

    diff = end - current;
    return diff <= 0;

}

void
update_timer(struct timer * tp, time_t sec, suseconds_t usec)
{
    if (gettimeofday(&tp->begin, NULL) < 0)
    {
	perror("gettimeofday");
    }
    memcpy(&tp->end, &tp->begin, sizeof(struct timer));
    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;
}

struct timer *
new_timer(time_t sec, suseconds_t usec)
{
    struct timer *tp;
    tp = (struct timer *) malloc(sizeof(struct timer));
    if (tp == NULL)
    {
	perror("malloc");
	return NULL;
    }

    if (gettimeofday(&tp->begin, NULL) < 0)
    {
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
free_timer(struct timer * tp)
{
    free(tp);
}

int
delay(int64_t ns)
{
    struct timespec req, rem;

    req.tv_sec = 0;

    while (ns >= 1000000000L)
    {
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
    if (gettimeofday(&now, NULL) < 0)
    {
	perror("gettimeofday");
	return -1;
    }
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
