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
 * Mutex.h
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * An abstract class for locking a mutex (mutual exclusion). If
 * threads are not available, this does nothing.
 * ------------------------------------------------------------------- */
#ifndef MUTEX_H
#define MUTEX_H

#include "headers.h"

#if   defined( HAVE_POSIX_THREAD )
    typedef pthread_mutex_t Mutex;
#elif defined( HAVE_WIN32_THREAD )
    typedef HANDLE Mutex;
#else
    typedef int Mutex;
#endif

/* ------------------------------------------------------------------- *
class Mutex {
public:*/
    
    // initialize mutex
#if   defined( HAVE_POSIX_THREAD )
    #define Mutex_Initialize( MutexPtr ) pthread_mutex_init( MutexPtr, NULL )
#elif defined( HAVE_WIN32_THREAD )
    #define Mutex_Initialize( MutexPtr ) *MutexPtr = CreateMutex( NULL, false, NULL )
#else
    #define Mutex_Initialize( MutexPtr )
#endif
    
    // lock the mutex variable
#if   defined( HAVE_POSIX_THREAD )
    #define Mutex_Lock( MutexPtr ) pthread_mutex_lock( MutexPtr )
#elif defined( HAVE_WIN32_THREAD )
    #define Mutex_Lock( MutexPtr ) WaitForSingleObject( *MutexPtr, INFINITE )
#else
    #define Mutex_Lock( MutexPtr )
#endif

    // unlock the mutex variable
#if   defined( HAVE_POSIX_THREAD )
    #define Mutex_Unlock( MutexPtr ) pthread_mutex_unlock( MutexPtr )
#elif defined( HAVE_WIN32_THREAD )
    #define Mutex_Unlock( MutexPtr ) ReleaseMutex( *MutexPtr )
#else
    #define Mutex_Unlock( MutexPtr )
#endif

    // destroy, making sure mutex is unlocked
#if   defined( HAVE_POSIX_THREAD )
    #define Mutex_Destroy( MutexPtr )  do {         \
        int rc = pthread_mutex_destroy( MutexPtr ); \
        if ( rc == EBUSY ) {                        \
            Mutex_Unlock( MutexPtr );               \
            pthread_mutex_destroy( MutexPtr );      \
        }                                           \
    } while ( 0 )
#elif defined( HAVE_WIN32_THREAD )
    #define Mutex_Destroy( MutexPtr ) CloseHandle( *MutexPtr )
#else
    #define Mutex_Destroy( MutexPtr )
#endif

#endif // MUTEX_H
