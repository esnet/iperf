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
 * by       Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * and      Mark Gates <mgates@nlanr.net>
 * ------------------------------------------------------------------- */

#define HEADERS()

#include "headers.h"

#include "SocketAddr.h"


#ifdef __cplusplus
extern "C" {
#endif
/* -------------------------------------------------------------------
 * Create a socket address. If inHostname is not null, resolve that
 * address and fill it in. Fill in the port number. Use IPv6 ADDR_ANY
 * if that is what is desired.
 * ------------------------------------------------------------------- */

void SockAddr_remoteAddr( thread_Settings *inSettings ) {
    SockAddr_zeroAddress( &inSettings->peer );
    if ( inSettings->mHost != NULL ) {
        SockAddr_setHostname( inSettings->mHost, &inSettings->peer, 
                              isIPV6( inSettings ) );
    } else {
#ifdef HAVE_IPV6
        if ( isIPV6( inSettings ) ) {
            ((struct sockaddr*)&inSettings->peer)->sa_family = AF_INET6;
        } else {
            ((struct sockaddr*)&inSettings->peer)->sa_family = AF_INET;
        }
    }

    if ( SockAddr_isIPv6( &inSettings->peer ) ) {
        inSettings->size_peer = sizeof( struct sockaddr_in6 );
    } else {
        inSettings->size_peer = sizeof( struct sockaddr_in );
    }
#else
        ((struct sockaddr*)&inSettings->peer)->sa_family = AF_INET;
    }
    inSettings->size_peer = sizeof( struct sockaddr_in );
#endif
    SockAddr_setPort( &inSettings->peer, inSettings->mPort );
}
// end SocketAddr

void SockAddr_localAddr( thread_Settings *inSettings ) {
    SockAddr_zeroAddress( &inSettings->local );
    if ( inSettings->mLocalhost != NULL ) {
        SockAddr_setHostname( inSettings->mLocalhost, &inSettings->local, 
                              isIPV6( inSettings ) );
    } else {
#ifdef HAVE_IPV6
        if ( isIPV6( inSettings ) ) {
            ((struct sockaddr*)&inSettings->local)->sa_family = AF_INET6;
        } else {
            ((struct sockaddr*)&inSettings->local)->sa_family = AF_INET;
        }
    }

    if ( SockAddr_isIPv6( &inSettings->local ) ) {
        inSettings->size_local = sizeof( struct sockaddr_in6 );
    } else {
        inSettings->size_local = sizeof( struct sockaddr_in );
    }
#else
        ((struct sockaddr*)&inSettings->local)->sa_family = AF_INET;
    }
        inSettings->size_local = sizeof( struct sockaddr_in );
#endif
    SockAddr_setPort( &inSettings->local, inSettings->mPort );
}
// end SocketAddr

/* -------------------------------------------------------------------
 * Resolve the hostname address and fill it in.
 * ------------------------------------------------------------------- */

void SockAddr_setHostname( const char* inHostname, 
                           iperf_sockaddr *inSockAddr, 
                           int isIPv6 ) {

    // ..I think this works for both ipv6 & ipv4... we'll see
#if defined(HAVE_IPV6)
    {
        struct addrinfo *res, *itr;
        int ret_ga;

        ret_ga = getaddrinfo(inHostname, NULL, NULL, &res);
        if ( ret_ga ) {
            fprintf(stderr, "error: %s\n", gai_strerror(ret_ga));
            exit(1);
        }
        if ( !res->ai_addr ) {
            fprintf(stderr, "getaddrinfo failed to get an address... target was '%s'\n", inHostname);
            exit(1);
        }

        // Check address type before filling in the address
        // ai_family = PF_xxx; ai_protocol = IPPROTO_xxx, see netdb.h
        // ...but AF_INET6 == PF_INET6
        itr = res;
        if ( isIPv6 ) {
            // First check all results for a IPv6 Address
            while ( itr != NULL ) {
                if ( itr->ai_family == AF_INET6 ) {
                    memcpy(inSockAddr, (itr->ai_addr),
                           (itr->ai_addrlen));
                    freeaddrinfo(res);
                    return;
                } else {
                    itr = itr->ai_next;
                }
            }
        }
        itr = res;
        // Now find a IPv4 Address
        while ( itr != NULL ) {
            if ( itr->ai_family == AF_INET ) {
                memcpy(inSockAddr, (itr->ai_addr),
                       (itr->ai_addrlen));
                freeaddrinfo(res);
                return;
            } else {
                itr = itr->ai_next;
            }
        }
    }
#else
    // first try just converting dotted decimal
    // on Windows gethostbyname doesn't understand dotted decimal
    int rc = inet_pton( AF_INET, inHostname, 
                        (unsigned char*)&(((struct sockaddr_in*)inSockAddr)->sin_addr) );
    inSockAddr->sin_family = AF_INET;
    if ( rc == 0 ) {
        struct hostent *hostP = gethostbyname( inHostname );
        if ( hostP == NULL ) {
            /* this is the same as herror() but works on more systems */
            const char* format;
            switch ( h_errno ) {
                case HOST_NOT_FOUND:
                    format = "%s: Unknown host\n";
                    break;
                case NO_ADDRESS:
                    format = "%s: No address associated with name\n";
                    break;
                case NO_RECOVERY:
                    format = "%s: Unknown server error\n";
                    break;
                case TRY_AGAIN:
                    format = "%s: Host name lookup failure\n";
                    break;

                default:
                    format = "%s: Unknown resolver error\n";
                    break;
            }
            fprintf( stderr, format, inHostname );
            exit(1);

            return; // TODO throw
        }

        memcpy(&(((struct sockaddr_in*)inSockAddr)->sin_addr), *(hostP->h_addr_list),
               (hostP->h_length));
    }
#endif
}
// end setHostname

/* -------------------------------------------------------------------
 * Copy the IP address into the string.
 * ------------------------------------------------------------------- */
void SockAddr_getHostAddress( iperf_sockaddr *inSockAddr, char* outAddress, 
                                size_t len ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET ) {
        inet_ntop( AF_INET, &(((struct sockaddr_in*) inSockAddr)->sin_addr), 
                   outAddress, len);
    }
#ifdef HAVE_IPV6
    else {
        inet_ntop( AF_INET6, &(((struct sockaddr_in6*) inSockAddr)->sin6_addr), 
                   outAddress, len);
    }
#endif
}
// end getHostAddress

/* -------------------------------------------------------------------
 * Set the address to any (generally all zeros).
 * ------------------------------------------------------------------- */

void SockAddr_setAddressAny( iperf_sockaddr *inSockAddr ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET )
        memset( &(((struct sockaddr_in*) inSockAddr)->sin_addr), 0, 
                sizeof( struct in_addr ));
#if defined(HAVE_IPV6)  
    else
        memset( &(((struct sockaddr_in6*) inSockAddr)->sin6_addr), 0, 
                sizeof( struct in6_addr ));
#endif
}
// end setAddressAny

/* -------------------------------------------------------------------
 * Set the port to the given port. Handles the byte swapping.
 * ------------------------------------------------------------------- */

void SockAddr_setPort( iperf_sockaddr *inSockAddr, unsigned short inPort ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET )
        ((struct sockaddr_in*) inSockAddr)->sin_port = htons( inPort );
#if defined(HAVE_IPV6)  
    else
        ((struct sockaddr_in6*) inSockAddr)->sin6_port = htons( inPort );
#endif

}
// end setPort

/* -------------------------------------------------------------------
 * Set the port to zero, which lets the OS pick the port.
 * ------------------------------------------------------------------- */

void SockAddr_setPortAny( iperf_sockaddr *inSockAddr ) {
    SockAddr_setPort( inSockAddr, 0 );
}
// end setPortAny

/* -------------------------------------------------------------------
 * Return the port. Handles the byte swapping.
 * ------------------------------------------------------------------- */

unsigned short SockAddr_getPort( iperf_sockaddr *inSockAddr ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET )
        return ntohs( ((struct sockaddr_in*) inSockAddr)->sin_port );
#if defined(HAVE_IPV6)
    else
        return ntohs( ((struct sockaddr_in6*) inSockAddr)->sin6_port);
#endif
    return 0;

}
// end getPort

/* -------------------------------------------------------------------
 * Return the IPv4 Internet Address from the sockaddr_in structure
 * ------------------------------------------------------------------- */

struct in_addr* SockAddr_get_in_addr( iperf_sockaddr *inSockAddr ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET )
        return &(((struct sockaddr_in*) inSockAddr)->sin_addr);

    fprintf(stderr, "FATAL: get_in_addr called on IPv6 address\n");
    return NULL;
}

/* -------------------------------------------------------------------
 * Return the IPv6 Internet Address from the sockaddr_in6 structure
 * ------------------------------------------------------------------- */
#ifdef HAVE_IPV6
struct in6_addr* SockAddr_get_in6_addr( iperf_sockaddr *inSockAddr ) {
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET6 )
        return &(((struct sockaddr_in6*) inSockAddr)->sin6_addr);

    fprintf(stderr, "FATAL: get_in6_addr called on IPv4 address\n");
    return NULL;
}
#endif


/* -------------------------------------------------------------------
 * Return the size of the appropriate address structure.
 * ------------------------------------------------------------------- */

Socklen_t SockAddr_get_sizeof_sockaddr( iperf_sockaddr *inSockAddr ) {

#if defined(HAVE_IPV6)
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET6 ) {
        return(sizeof(struct sockaddr_in6));
    }
#endif
    return(sizeof(struct sockaddr_in));
}
// end get_sizeof_sockaddr


/* -------------------------------------------------------------------
 * Return if IPv6 socket
 * ------------------------------------------------------------------- */

int SockAddr_isIPv6( iperf_sockaddr *inSockAddr ) {

#if defined(HAVE_IPV6)
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET6 ) {
        return 1;
    }
#endif
    return 0;
}
// end get_sizeof_sockaddr

/* -------------------------------------------------------------------
 * Return true if the address is a IPv4 multicast address.
 * ------------------------------------------------------------------- */

int SockAddr_isMulticast( iperf_sockaddr *inSockAddr ) {

#if defined(HAVE_IPV6)
    if ( ((struct sockaddr*)inSockAddr)->sa_family == AF_INET6 ) {
        return( IN6_IS_ADDR_MULTICAST(&(((struct sockaddr_in6*) inSockAddr)->sin6_addr) ));
    } else
#endif
    {
        // 224.0.0.0 to 239.255.255.255 (e0.00.00.00 to ef.ff.ff.ff)
        const unsigned long kMulticast_Mask = 0xe0000000L;

        return(kMulticast_Mask ==
               (ntohl( ((struct sockaddr_in*) inSockAddr)->sin_addr.s_addr) & kMulticast_Mask));
    }
}
// end isMulticast

/* -------------------------------------------------------------------
 * Zero out the address structure.
 * ------------------------------------------------------------------- */

void SockAddr_zeroAddress( iperf_sockaddr *inSockAddr ) {
    memset( inSockAddr, 0, sizeof( iperf_sockaddr ));
}
// zeroAddress

/* -------------------------------------------------------------------
 * Compare two sockaddrs and return true if they are equal
 * ------------------------------------------------------------------- */
int SockAddr_are_Equal( struct sockaddr* first, struct sockaddr* second ) {
    if ( first->sa_family == AF_INET && second->sa_family == AF_INET ) {
        // compare IPv4 adresses
        return( ((long) ((struct sockaddr_in*)first)->sin_addr.s_addr == (long) ((struct sockaddr_in*)second)->sin_addr.s_addr)
                && ( ((struct sockaddr_in*)first)->sin_port == ((struct sockaddr_in*)second)->sin_port) );
    }
#if defined(HAVE_IPV6)      
    if ( first->sa_family == AF_INET6 && second->sa_family == AF_INET6 ) {
        // compare IPv6 addresses
        return( !memcmp(((struct sockaddr_in6*)first)->sin6_addr.s6_addr, ((struct sockaddr_in6*)second)->sin6_addr.s6_addr, sizeof(struct in6_addr)) 
                && (((struct sockaddr_in6*)first)->sin6_port == ((struct sockaddr_in6*)second)->sin6_port) );
    }
#endif 
    return 0;

}

/* -------------------------------------------------------------------
 * Compare two sockaddrs and return true if the hosts are equal
 * ------------------------------------------------------------------- */
int SockAddr_Hostare_Equal( struct sockaddr* first, struct sockaddr* second ) {
    if ( first->sa_family == AF_INET && second->sa_family == AF_INET ) {
        // compare IPv4 adresses
        return( (long) ((struct sockaddr_in*)first)->sin_addr.s_addr == 
                (long) ((struct sockaddr_in*)second)->sin_addr.s_addr);
    }
#if defined(HAVE_IPV6)      
    if ( first->sa_family == AF_INET6 && second->sa_family == AF_INET6 ) {
        // compare IPv6 addresses
        return( !memcmp(((struct sockaddr_in6*)first)->sin6_addr.s6_addr, 
                        ((struct sockaddr_in6*)second)->sin6_addr.s6_addr, sizeof(struct in6_addr)));
    }
#endif 
    return 0;

}
#ifdef __cplusplus
} /* end extern "C" */
#endif
