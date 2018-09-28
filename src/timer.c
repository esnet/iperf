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
 *
 * Based on timers.c by Jef Poskanzer. Used with permission.
 */

#include <sys/types.h>
#include <stdlib.h>

#include "timer.h"
#include "iperf_time.h"

static Timer* timers = NULL;
static Timer* free_timers = NULL;

TimerClientData JunkClientData;



/* This is an efficiency tweak.  All the routines that need to know the
** current time get passed a pointer to a struct iperf_time.  If it's non-NULL
** it gets used, otherwise we do our own iperf_time_now() to fill it in.
** This lets the caller avoid extraneous iperf_time_now()s when efficiency
** is needed, and not bother with the extra code when efficiency doesn't
** matter too much.
*/
static void
getnow( struct iperf_time* nowP, struct iperf_time* nowP2 )
{
    if ( nowP != NULL )
	*nowP2 = *nowP;
    else
	iperf_time_now(nowP2);
}


static void
list_add( Timer* t )
{
    Timer* t2;
    Timer* t2prev;

    if ( timers == NULL ) {
	/* The list is empty. */
	timers = t;
	t->prev = t->next = NULL;
    } else {
	if (iperf_time_compare(&t->time, &timers->time) < 0) {
	    /* The new timer goes at the head of the list. */
	    t->prev = NULL;
	    t->next = timers;
	    timers->prev = t;
	    timers = t;
	} else {
	    /* Walk the list to find the insertion point. */
	    for ( t2prev = timers, t2 = timers->next; t2 != NULL;
		  t2prev = t2, t2 = t2->next ) {
		if (iperf_time_compare(&t->time, &t2->time) < 0) {
		    /* Found it. */
		    t2prev->next = t;
		    t->prev = t2prev;
		    t->next = t2;
		    t2->prev = t;
		    return;
		}
	    }
	    /* Oops, got to the end of the list.  Add to tail. */
	    t2prev->next = t;
	    t->prev = t2prev;
	    t->next = NULL;
	}
    }
}


static void
list_remove( Timer* t )
{
    if ( t->prev == NULL )
	timers = t->next;
    else
	t->prev->next = t->next;
    if ( t->next != NULL )
	t->next->prev = t->prev;
}


static void
list_resort( Timer* t )
{
    /* Remove the timer from the list. */
    list_remove( t );
    /* And add it back in, sorted correctly. */
    list_add( t );
}


Timer*
tmr_create(
    struct iperf_time* nowP, TimerProc* timer_proc, TimerClientData client_data,
    int64_t usecs, int periodic )
{
    struct iperf_time now;
    Timer* t;

    getnow( nowP, &now );

    if ( free_timers != NULL ) {
	t = free_timers;
	free_timers = t->next;
    } else {
	t = (Timer*) malloc( sizeof(Timer) );
	if ( t == NULL )
	    return NULL;
    }

    t->timer_proc = timer_proc;
    t->client_data = client_data;
    t->usecs = usecs;
    t->periodic = periodic;
    t->time = now;
    iperf_time_add_usecs(&t->time, usecs);
    /* Add the new timer to the active list. */
    list_add( t );

    return t;
}


struct timeval*
tmr_timeout( struct iperf_time* nowP )
{
    struct iperf_time now, diff;
    int64_t usecs;
    int past;
    static struct timeval timeout;

    getnow( nowP, &now );
    /* Since the list is sorted, we only need to look at the first timer. */
    if ( timers == NULL )
	return NULL;
    past = iperf_time_diff(&timers->time, &now, &diff);
    if (past)
        usecs = 0;
    else
        usecs = iperf_time_in_usecs(&diff);
    timeout.tv_sec = usecs / 1000000LL;
    timeout.tv_usec = usecs % 1000000LL;
    return &timeout;
}


void
tmr_run( struct iperf_time* nowP )
{
    struct iperf_time now;
    Timer* t;
    Timer* next;

    getnow( nowP, &now );
    for ( t = timers; t != NULL; t = next ) {
	next = t->next;
	/* Since the list is sorted, as soon as we find a timer
	** that isn't ready yet, we are done.
	*/
	if (iperf_time_compare(&t->time, &now) > 0)
	    break;
	(t->timer_proc)( t->client_data, &now );
	if ( t->periodic ) {
	    /* Reschedule. */
	    iperf_time_add_usecs(&t->time, t->usecs);
	    list_resort( t );
	} else
	    tmr_cancel( t );
    }
}


void
tmr_reset( struct iperf_time* nowP, Timer* t )
{
    struct iperf_time now;
    
    getnow( nowP, &now );
    t->time = now;
    iperf_time_add_usecs( &t->time, t->usecs );
    list_resort( t );
}


void
tmr_cancel( Timer* t )
{
    /* Remove it from the active list. */
    list_remove( t );
    /* And put it on the free list. */
    t->next = free_timers;
    free_timers = t;
    t->prev = NULL;
}


void
tmr_cleanup( void )
{
    Timer* t;

    while ( free_timers != NULL ) {
	t = free_timers;
	free_timers = t->next;
	free( (void*) t );
    }
}


void
tmr_destroy( void )
{
    while ( timers != NULL )
	tmr_cancel( timers );
    tmr_cleanup();
}
