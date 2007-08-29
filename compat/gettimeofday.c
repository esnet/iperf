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
 * gettimeofday.c
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * A (hack) implementation of gettimeofday for Windows.
 * Since I send sec/usec in UDP packets, this made the most sense.
 * ------------------------------------------------------------------- */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifndef HAVE_GETTIMEOFDAY

    #include "headers.h"
    #include "gettimeofday.h"

    #ifdef __cplusplus
extern "C" {
#endif

int gettimeofday( struct timeval* tv, void* timezone ) {
    FILETIME time;
    double   timed;

    GetSystemTimeAsFileTime( &time );

    // Apparently Win32 has units of 1e-7 sec (tenths of microsecs)
    // 4294967296 is 2^32, to shift high word over
    // 11644473600 is the number of seconds between
    // the Win32 epoch 1601-Jan-01 and the Unix epoch 1970-Jan-01
    // Tests found floating point to be 10x faster than 64bit int math.

    timed = ((time.dwHighDateTime * 4294967296e-7) - 11644473600.0) +
            (time.dwLowDateTime  * 1e-7);

    tv->tv_sec  = (long) timed;
    tv->tv_usec = (long) ((timed - tv->tv_sec) * 1e6);

    return 0;
}

#ifdef __cplusplus
} /* end extern "C" */
    #endif

#endif /* HAVE_GETTIMEOFDAY */
