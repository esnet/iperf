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
 * Socket.cpp
 * by Ajay Tirumala <tirumala@ncsa.uiuc.edu> 
 *    and Mark Gates <mgates@nlanr.net>
 * ------------------------------------------------------------------- */


#ifndef SOCKET_ADDR_H
#define SOCKET_ADDR_H

#include "headers.h"
#include "Settings.hpp"


#ifdef __cplusplus
extern "C" {
#endif
/* ------------------------------------------------------------------- */
    void SockAddr_localAddr( thread_Settings *inSettings );
    void SockAddr_remoteAddr( thread_Settings *inSettings );

    void SockAddr_setHostname( const char* inHostname,
                               iperf_sockaddr *inSockAddr, 
                               int isIPv6 );          // DNS lookup
    void SockAddr_getHostname( iperf_sockaddr *inSockAddr,
                               char* outHostname, 
                               size_t len );   // reverse DNS lookup
    void SockAddr_getHostAddress( iperf_sockaddr *inSockAddr, 
                                  char* outAddress, 
                                  size_t len ); // dotted decimal

    void SockAddr_setPort( iperf_sockaddr *inSockAddr, unsigned short inPort );
    void SockAddr_setPortAny( iperf_sockaddr *inSockAddr );
    unsigned short SockAddr_getPort( iperf_sockaddr *inSockAddr );

    void SockAddr_setAddressAny( iperf_sockaddr *inSockAddr );

    // return pointer to the struct in_addr
    struct in_addr* SockAddr_get_in_addr( iperf_sockaddr *inSockAddr );
#ifdef HAVE_IPV6
    // return pointer to the struct in_addr
    struct in6_addr* SockAddr_get_in6_addr( iperf_sockaddr *inSockAddr );
#endif
    // return the sizeof the addess structure (struct sockaddr_in)
    Socklen_t SockAddr_get_sizeof_sockaddr( iperf_sockaddr *inSockAddr );

    int SockAddr_isMulticast( iperf_sockaddr *inSockAddr );

    int SockAddr_isIPv6( iperf_sockaddr *inSockAddr );

    int SockAddr_are_Equal( struct sockaddr *first, struct sockaddr *second );
    int SockAddr_Hostare_Equal( struct sockaddr *first, struct sockaddr *second );

    void SockAddr_zeroAddress( iperf_sockaddr *inSockAddr );
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* SOCKET_ADDR_H */
