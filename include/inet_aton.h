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
 * inet_aton.h
 *
 * Mark Gates <mgates@nlanr.net>
 *      Kevin Gibbs <kgibbs@ncsa.uiuc.edu> Sept. 2002
 * to use this prototype, make sure HAVE_INET_PTON is not defined
 * to use this prototype, make sure HAVE_INET_NTOP is not defined
 *
 * =================================================================== */

#ifndef INET_ATON_H
#define INET_ATON_H


#include "headers.h"

/*
 * inet_pton is the new, better version of inet_aton.
 * inet_aton is not IP version agnostic.
 * inet_aton is the new, better version of inet_addr.
 * inet_addr is incorrect in that it returns -1 as an error value,
 * while -1 (0xFFFFFFFF) is a valid IP address (255.255.255.255).
 */

#ifndef HAVE_INET_NTOP

    #ifdef __cplusplus
extern "C" {
#endif
int inet_ntop(int af, const void *src, char *dst, size_t size);
int inet_ntop4(const unsigned char *src, char *dst,
                      size_t size);
#ifdef HAVE_IPV6
int inet_ntop6(const unsigned char *src, char *dst,
                      size_t size);
#endif


#ifdef __cplusplus
} /* end extern "C" */
    #endif

#endif /* HAVE_INET_NTOP */
#ifndef HAVE_INET_PTON

    #ifdef __cplusplus
extern "C" {
#endif
int inet_pton(int af, const char *src, void *dst);
int inet_pton4(const char *src, unsigned char *dst);
#ifdef HAVE_IPV6
int inet_pton6(const char *src, unsigned char *dst);
#endif

#ifdef __cplusplus
} /* end extern "C" */
    #endif

#endif /* HAVE_INET_PTON */
#endif /* INET_ATON_H */
