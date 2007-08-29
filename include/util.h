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
 * util.h
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * various C utility functions.
 * ------------------------------------------------------------------- */

#ifndef UTIL_H
#define UTIL_H

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------
 * set/getsockopt wrappers for SO_RCVBUF and SO_SNDBUF; TCP_MAXSEG
 * socket.c
 * ------------------------------------------------------------------- */
int setsock_tcp_windowsize( int inSock, int inTCPWin, int inSend );
int getsock_tcp_windowsize( int inSock, int inSend );

void setsock_tcp_mss( int inSock, int inTCPWin );
int  getsock_tcp_mss( int inSock );

/* -------------------------------------------------------------------
 * signal handlers
 * signal.c
 * ------------------------------------------------------------------- */
typedef void Sigfunc(int);
void sig_exit( int inSigno );

typedef Sigfunc *SigfuncPtr;

SigfuncPtr my_signal( int inSigno, SigfuncPtr inFunc );

#ifdef WIN32

/* under windows, emulate unix signals */
enum {
    SIGINT,
    SIGTERM,
    SIGPIPE,
    _NSIG
};

BOOL WINAPI sig_dispatcher( DWORD type );

#endif

/* -------------------------------------------------------------------
 * error handlers
 * error.c
 * ------------------------------------------------------------------- */
void warn      ( const char *inMessage, const char *inFile, int inLine );
void warn_errno( const char *inMessage, const char *inFile, int inLine );

#if defined( HAVE_POSIX_THREAD ) || defined( HAVE_WIN32_THREAD)
#define FAIL( cond, msg, settings )             \
  do {                                          \
    if ( cond ) {                               \
      warn( msg, __FILE__, __LINE__ );          \
      thread_stop(settings);                    \
    }                                           \
  } while( 0 )
#else
#define FAIL( cond, msg, settings )             \
  do {                                          \
    if ( cond ) {                               \
      warn( msg, __FILE__, __LINE__ );          \
      exit( 1 );                                \
    }                                           \
  } while( 0 )
#endif

#define WARN( cond, msg )                       \
  do {                                          \
    if ( cond ) {                               \
      warn( msg, __FILE__, __LINE__ );          \
    }                                           \
  } while( 0 )

#if defined( HAVE_POSIX_THREAD ) || defined( HAVE_WIN32_THREAD)
#define FAIL_errno( cond, msg, settings )       \
  do {                                          \
    if ( cond ) {                               \
      warn_errno( msg, __FILE__, __LINE__ );    \
      thread_stop(settings);                    \
    }                                           \
  } while( 0 )
#else
#define FAIL_errno( cond, msg, settings )       \
  do {                                          \
    if ( cond ) {                               \
      warn_errno( msg, __FILE__, __LINE__ );    \
      exit( 1 );                                \
    }                                           \
  } while( 0 )
#endif

#define WARN_errno( cond, msg )                 \
  do {                                          \
    if ( cond ) {                               \
      warn_errno( msg, __FILE__, __LINE__ );    \
    }                                           \
  } while( 0 )

/* -------------------------------------------------------------------
 * initialize buffer to a pattern
 * ------------------------------------------------------------------- */
void pattern( char *outBuf, int inBytes );

/* -------------------------------------------------------------------
 * input and output numbers, converting with kilo, mega, giga
 * stdio.c
 * ------------------------------------------------------------------- */
double byte_atof( const char *inString );
max_size_t byte_atoi( const char  *inString );
void byte_snprintf( char* outString, int inLen, double inNum, char inFormat );

/* -------------------------------------------------------------------
 * redirect the stdout to a specified file
 * stdio.c
 * ------------------------------------------------------------------- */
void redirect(const char *inOutputFileName);

/* -------------------------------------------------------------------
 * delete macro
 * ------------------------------------------------------------------- */
#define DELETE_PTR( ptr )                       \
  do {                                          \
    if ( ptr != NULL ) {                        \
      delete ptr;                               \
      ptr = NULL;                               \
    }                                           \
  } while( false )

#define DELETE_ARRAY( ptr )                     \
  do {                                          \
    if ( ptr != NULL ) {                        \
      delete [] ptr;                            \
      ptr = NULL;                               \
    }                                           \
  } while( false )

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* UTIL_H */

