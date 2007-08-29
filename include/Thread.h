/*--------------------------------------------------------------- 
 * Copyright (c) 1999,2000,2001,2002,2003                              
 * The Board of Trustees of the University of Illinois            
 * All Rights Reserved.                                           
 *--------------------------------------------------------------- 
 * Permission is hereby granted, free of charge, to any person    
 * obtaining a copy of this software (Iperf) and associated       
 * documentation files (the "Software"), to deal in the Software  
 * without restriction, including without limitation the          
 * rights to use, copy, modify, merge, publish, distribute,        
 * sublicense, and/or sell copies of the Software, and to permit     
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions: 
 *
 *     
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and 
 * the following disclaimers. 
 *
 *     
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimers in the documentation and/or other materials 
 * provided with the distribution. 
 * 
 *     
 * Neither the names of the University of Illinois, NCSA, 
 * nor the names of its contributors may be used to endorse 
 * or promote products derived from this Software without
 * specific prior written permission. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * ________________________________________________________________
 * National Laboratory for Applied Network Research 
 * National Center for Supercomputing Applications 
 * University of Illinois at Urbana-Champaign 
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________ 
 *
 * Thread.h
 * by Kevin Gibbs <kgibbs@nlanr.net>
 *
 * Based on:
 * Thread.hpp
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * The thread subsystem is responsible for all thread functions. It
 * provides a thread implementation agnostic interface to Iperf. If 
 * threads are not available (HAVE_THREAD is undefined), thread_start
 * does not start a new thread but just launches the specified object 
 * in the current thread. Everything that defines a thread of 
 * execution in Iperf is contained in an thread_Settings structure. To
 * start a thread simply pass one such structure into thread_start.
 * ------------------------------------------------------------------- */

#ifndef THREAD_H
#define THREAD_H

#ifdef __cplusplus
extern "C" {
#endif


#if   defined( HAVE_POSIX_THREAD )

/* Definitions for Posix Threads (pthreads) */
    #include <pthread.h>

typedef pthread_t nthread_t;

    #define HAVE_THREAD 1

#elif defined( HAVE_WIN32_THREAD )

/* Definitions for Win32 NT Threads */
typedef DWORD nthread_t;

    #define HAVE_THREAD 1

#else

/* Definitions for no threads */
typedef int nthread_t;

    #undef HAVE_THREAD

#endif

    // Forward declaration
    struct thread_Settings;

#include "Condition.h"
#include "Settings.hpp"

    // initialize or destroy the thread subsystem
    void thread_init( );
    void thread_destroy( );

    // start or stop a thread executing
    void thread_start( struct thread_Settings* thread );
    void thread_stop( struct thread_Settings* thread );

    /* wait for this or all threads to complete */
    void thread_joinall( void );

    int thread_numuserthreads( void );

    // set a thread to be ignorable, so joinall won't wait on it
    void thread_setignore( void );
    void thread_unsetignore( void );

    // Used for threads that may never terminate (ie Listener Thread)
    void thread_register_nonterm( void );
    void thread_unregister_nonterm( void );
    int thread_release_nonterm( int force );

    /* -------------------------------------------------------------------
     * Return the current thread's ID.
     * ------------------------------------------------------------------- */
    #if   defined( HAVE_POSIX_THREAD )
        #define thread_getid() pthread_self()
    #elif defined( HAVE_WIN32_THREAD )
        #define thread_getid() GetCurrentThreadId()
    #else
        #define thread_getid() 0
    #endif

    int thread_equalid( nthread_t inLeft, nthread_t inRight );

    nthread_t thread_zeroid( void );
    
#if   defined( HAVE_WIN32_THREAD )
    DWORD WINAPI thread_run_wrapper( void* paramPtr );
#else
    void*        thread_run_wrapper( void* paramPtr );
#endif

    void thread_rest ( void );

    // defined in launch.cpp
    void server_spawn( struct thread_Settings* thread );
    void client_spawn( struct thread_Settings* thread );
    void client_init( struct thread_Settings* clients );
    void listener_spawn( struct thread_Settings* thread );

    // defined in reporter.c
    void reporter_spawn( struct thread_Settings* thread );

#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif // THREAD_H
