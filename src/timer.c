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
timeval_to_double(struct timeval *tv)
{
    double d;
    
    d = tv->tv_sec + tv->tv_usec /1000000;    
    
    return d;
}

double
timeval_diff(struct timeval *tv0, struct timeval *tv1)
{
    return timeval_to_double(tv1) - timeval_to_double(tv0);
}

int
timer_expired(struct timer *tp)
{ 
    struct timeval now;
    int64_t  end = 0, current= 0, diff= 0;
    if(gettimeofday(&now, NULL) < 0) {
        perror("gettimeofday");
        return -1;
    }
    
    end+= tp->end.tv_sec * 1000000 ;
    end+= tp->end.tv_usec;
    
    current+= now.tv_sec * 1000000 ;
    current+= now.tv_usec;
    
    diff = end - current;    
    
   return diff <= 0;
    
    // currently using microsecond limit. Else we need to introduce timespec instread of timeval
} 

void
update_timer(struct timer *tp, time_t sec, suseconds_t usec)
{
    if(gettimeofday(&tp->begin, NULL) < 0) {
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

int64_t 
timer_remaining(struct timer *tp)
{
    struct timeval now;
    int64_t  end = 0, current= 0, diff= 0;
    if(gettimeofday(&now, NULL) < 0) {
        perror("gettimeofday");
        return -1;
    }
    
    end+= tp->end.tv_sec * 1000000 ;
    end+= tp->end.tv_usec;
    
    current+= now.tv_sec * 1000000 ;
    current+= now.tv_usec;
    
    diff = end - current;
    if(diff > 0)
        return diff;
    else
        return 0;
}
    

