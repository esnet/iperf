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
 * headers.h
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * All system headers required by iperf.
 * This could be processed to form a single precompiled header,
 * to avoid overhead of compiling it multiple times.
 * This also verifies a few things are defined, for portability.
 * ------------------------------------------------------------------- */

#ifndef HEADERS_H
#define HEADERS_H


#ifdef HAVE_CONFIG_H
    #include "config.h"

/* OSF1 (at least the system I use) needs extern C
 * around the <netdb.h> and <arpa/inet.h> files. */
    #if defined( SPECIAL_OSF1_EXTERN ) && defined( __cplusplus )
        #define SPECIAL_OSF1_EXTERN_C_START extern "C" {
        #define SPECIAL_OSF1_EXTERN_C_STOP  }
    #else
        #define SPECIAL_OSF1_EXTERN_C_START
        #define SPECIAL_OSF1_EXTERN_C_STOP
    #endif
#endif /* HAVE_CONFIG_H */

/* turn off assert debugging */
#define NDEBUG

/* standard C headers */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef WIN32

/* Windows config file */
    #include "config.win32.h"

/* Windows headers */
    #define _WIN32_WINNT 0x0400 /* use (at least) WinNT 4.0 API */
    #define WIN32_LEAN_AND_MEAN /* exclude unnecesary headers */
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

/* define EINTR, just to help compile; it isn't useful */
    #ifndef EINTR
        #define EINTR  WSAEINTR
    #endif // EINTR

/* Visual C++ has INT64, but not 'long long'.
 * Metrowerks has 'long long', but INT64 doesn't work. */
    #ifdef __MWERKS__
        #define int64_t  long long 
    #else
        #define int64_t  INT64
    #endif // __MWERKS__

/* Visual C++ has _snprintf instead of snprintf */
    #ifndef __MWERKS__
        #define snprintf _snprintf
    #endif // __MWERKS__

/* close, read, and write only work on files in Windows.
 * I get away with #defining them because I don't read files. */
    #define close( s )       closesocket( s )
    #define read( s, b, l )  recv( s, (char*) b, l, 0 )
    #define write( s, b, l ) send( s, (char*) b, l, 0 )

#else /* not defined WIN32 */

/* required on AIX for FD_SET (requires bzero).
 * often this is the same as <string.h> */
    #ifdef HAVE_STRINGS_H
        #include <strings.h>
    #endif // HAVE_STRINGS_H

/* unix headers */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <signal.h>
    #include <unistd.h>

/** Added for daemonizing the process */
    #include <syslog.h>

SPECIAL_OSF1_EXTERN_C_START
    #include <netdb.h>
SPECIAL_OSF1_EXTERN_C_STOP
    #include <netinet/in.h>
    #include <netinet/tcp.h>

SPECIAL_OSF1_EXTERN_C_START
    #include <arpa/inet.h>   /* netinet/in.h must be before this on SunOS */
SPECIAL_OSF1_EXTERN_C_STOP

    #ifdef HAVE_POSIX_THREAD
        #include <pthread.h>
    #endif // HAVE_POSIX_THREAD

/* used in Win32 for error checking,
 * rather than checking rc < 0 as unix usually does */
    #define SOCKET_ERROR   -1
    #define INVALID_SOCKET -1

#endif /* not defined WIN32 */

#ifndef INET6_ADDRSTRLEN
    #define INET6_ADDRSTRLEN 40
#endif
#ifndef INET_ADDRSTRLEN
    #define INET_ADDRSTRLEN 15
#endif

//#ifdef __cplusplus
    #ifdef HAVE_IPV6
        #define REPORT_ADDRLEN (INET6_ADDRSTRLEN + 1)
typedef struct sockaddr_storage iperf_sockaddr;
    #else
        #define REPORT_ADDRLEN (INET_ADDRSTRLEN + 1)
typedef struct sockaddr_in iperf_sockaddr;
    #endif
//#endif

// Rationalize stdint definitions and sizeof, thanks to ac_create_stdint_h.m4
// from the gnu archive

#include <iperf-int.h>
typedef uint64_t max_size_t;

/* in case the OS doesn't have these, we provide our own implementations */
#include "gettimeofday.h"
#include "inet_aton.h"
#include "snprintf.h"

#ifndef SHUT_RD
    #define SHUT_RD   0
    #define SHUT_WR   1
    #define SHUT_RDWR 2
#endif // SHUT_RD

#endif /* HEADERS_H */






