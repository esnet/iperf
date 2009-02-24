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
 * socket.c
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * set/getsockopt and read/write wrappers
 * ------------------------------------------------------------------- */

#include "headers.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------
 * If inMSS > 0, set the TCP maximum segment size  for inSock.
 * Otherwise leave it as the system default.
 * ------------------------------------------------------------------- */

const char warn_mss_fail[] = "\
WARNING: attempt to set TCP maxmimum segment size to %d failed.\n\
Setting the MSS may not be implemented on this OS.\n";

const char warn_mss_notset[] =
"WARNING: attempt to set TCP maximum segment size to %d, but got %d\n";

void setsock_tcp_mss( int inSock, int inMSS ) {
#ifdef TCP_MAXSEG
    int rc;
    int newMSS;
    Socklen_t len;

    assert( inSock != INVALID_SOCKET );

    if ( inMSS > 0 ) {
        /* set */
        newMSS = inMSS;
        len = sizeof( newMSS );
        rc = setsockopt( inSock, IPPROTO_TCP, TCP_MAXSEG, (char*) &newMSS,  len );
        if ( rc == SOCKET_ERROR ) {
            fprintf( stderr, warn_mss_fail, newMSS );
            return;
        }

        /* verify results */
        rc = getsockopt( inSock, IPPROTO_TCP, TCP_MAXSEG, (char*) &newMSS, &len );
        WARN_errno( rc == SOCKET_ERROR, "getsockopt TCP_MAXSEG" );
        if ( newMSS != inMSS ) {
            fprintf( stderr, warn_mss_notset, inMSS, newMSS );
        }
    }
#endif
} /* end setsock_tcp_mss */

/* -------------------------------------------------------------------
 * returns the TCP maximum segment size
 * ------------------------------------------------------------------- */

int getsock_tcp_mss( int inSock ) {
    int theMSS = 0;

#ifdef TCP_MAXSEG
    int rc;
    Socklen_t len;
    assert( inSock >= 0 );

    /* query for MSS */
    len = sizeof( theMSS );
    rc = getsockopt( inSock, IPPROTO_TCP, TCP_MAXSEG, (char*) &theMSS, &len );
    WARN_errno( rc == SOCKET_ERROR, "getsockopt TCP_MAXSEG" );
#endif

    return theMSS;
} /* end getsock_tcp_mss */

/* -------------------------------------------------------------------
 * Attempts to reads n bytes from a socket.
 * Returns number actually read, or -1 on error.
 * If number read < inLen then we reached EOF.
 *
 * from Stevens, 1998, section 3.9
 * ------------------------------------------------------------------- */

ssize_t readn( int inSock, void *outBuf, size_t inLen ) {
    size_t  nleft;
    ssize_t nread;
    char *ptr;

    assert( inSock >= 0 );
    assert( outBuf != NULL );
    assert( inLen > 0 );

    ptr   = (char*) outBuf;
    nleft = inLen;

    while ( nleft > 0 ) {
        nread = read( inSock, ptr, nleft );
        if ( nread < 0 ) {
            if ( errno == EINTR )
                nread = 0;  /* interupted, call read again */
            else
                return -1;  /* error */
        } else if ( nread == 0 )
            break;        /* EOF */

        nleft -= nread;
        ptr   += nread;
    }

    return(inLen - nleft);
} /* end readn */

/* -------------------------------------------------------------------
 * Attempts to write  n bytes to a socket.
 * returns number actually written, or -1 on error.
 * number written is always inLen if there is not an error.
 *
 * from Stevens, 1998, section 3.9
 * ------------------------------------------------------------------- */

ssize_t writen( int inSock, const void *inBuf, size_t inLen ) {
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    assert( inSock >= 0 );
    assert( inBuf != NULL );
    assert( inLen > 0 );

    ptr   = (char*) inBuf;
    nleft = inLen;

    while ( nleft > 0 ) {
        nwritten = write( inSock, ptr, nleft );
        if ( nwritten <= 0 ) {
            if ( errno == EINTR )
                nwritten = 0; /* interupted, call write again */
            else
                return -1;    /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }

    return inLen;
} /* end writen */

#ifdef __cplusplus
} /* end extern "C" */
#endif
