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
 * Listener.cpp
 * by Mark Gates <mgates@nlanr.net> 
 * &  Ajay Tirumala <tirumala@ncsa.uiuc.edu> 
 * ------------------------------------------------------------------- 
 * Listener sets up a socket listening on the server host. For each 
 * connected socket that accept() returns, this creates a Server 
 * socket and spawns a thread for it. 
 * 
 * Changes to the latest version. Listener will run as a daemon 
 * Multicast Server is now Multi-threaded 
 * ------------------------------------------------------------------- 
 * headers 
 * uses 
 *   <stdlib.h> 
 *   <stdio.h> 
 *   <string.h> 
 *   <errno.h> 
 * 
 *   <sys/types.h> 
 *   <unistd.h> 
 * 
 *   <netdb.h> 
 *   <netinet/in.h> 
 *   <sys/socket.h> 
 * ------------------------------------------------------------------- */ 


#define HEADERS() 

#include "headers.h" 
#include "Listener.hpp"
#include "SocketAddr.h"
#include "PerfSocket.hpp"
#include "List.h"
#include "util.h" 

/* ------------------------------------------------------------------- 
 * Stores local hostname and socket info. 
 * ------------------------------------------------------------------- */ 

Listener::Listener( thread_Settings *inSettings ) {

    mClients = inSettings->mThreads;
    mBuf = NULL;
    mSettings = inSettings;

    // initialize buffer
    mBuf = new char[ mSettings->mBufLen ];

    // open listening socket 
    Listen( ); 
    ReportSettings( inSettings );

} // end Listener 

/* ------------------------------------------------------------------- 
 * Delete memory (buffer). 
 * ------------------------------------------------------------------- */ 
Listener::~Listener() {
    if ( mSettings->mSock != INVALID_SOCKET ) {
        int rc = close( mSettings->mSock );
        WARN_errno( rc == SOCKET_ERROR, "close" );
        mSettings->mSock = INVALID_SOCKET;
    }
    DELETE_ARRAY( mBuf );
} // end ~Listener 

/* ------------------------------------------------------------------- 
 * Listens for connections and starts Servers to handle data. 
 * For TCP, each accepted connection spawns a Server thread. 
 * For UDP, handle all data in this thread for Win32 Only, otherwise
 *          spawn a new Server thread. 
 * ------------------------------------------------------------------- */ 
void Listener::Run( void ) {
#ifdef WIN32
    if ( isUDP( mSettings ) && !isSingleUDP( mSettings ) ) {
        UDPSingleServer();
    } else
#else
#ifdef sun
    if ( ( isUDP( mSettings ) && 
           isMulticast( mSettings ) && 
           !isSingleUDP( mSettings ) ) ||
         isSingleUDP( mSettings ) ) {
        UDPSingleServer();
    } else
#else
    if ( isSingleUDP( mSettings ) ) {
        UDPSingleServer();
    } else
#endif
#endif
    {
        bool client = false, UDP = isUDP( mSettings ), mCount = (mSettings->mThreads != 0);
        thread_Settings *tempSettings = NULL;
        Iperf_ListEntry *exist, *listtemp;
        client_hdr* hdr = ( UDP ? (client_hdr*) (((UDP_datagram*)mBuf) + 1) : 
                                  (client_hdr*) mBuf);
        
        if ( mSettings->mHost != NULL ) {
            client = true;
            SockAddr_remoteAddr( mSettings );
        }
        Settings_Copy( mSettings, &server );
        server->mThreadMode = kMode_Server;
    
    
        // Accept each packet, 
        // If there is no existing client, then start  
        // a new thread to service the new client 
        // The listener runs in a single thread 
        // Thread per client model is followed 
        do {
            // Get a new socket
            Accept( server );
            if ( server->mSock == INVALID_SOCKET ) {
                break;
            }
            if ( sInterupted != 0 ) {
                close( server->mSock );
                break;
            }
            // Reset Single Client Stuff
            if ( isSingleClient( mSettings ) && clients == NULL ) {
                mSettings->peer = server->peer;
                mClients--;
                client = true;
                // Once all the server threads exit then quit
                // Must keep going in case this client sends
                // more streams
                if ( mClients == 0 ) {
                    thread_release_nonterm( 0 );
                    mClients = 1;
                }
            }
            // Verify that it is allowed
            if ( client ) {
                if ( !SockAddr_Hostare_Equal( (sockaddr*) &mSettings->peer, 
                                              (sockaddr*) &server->peer ) ) {
                    // Not allowed try again
                    close( server->mSock );
                    if ( isUDP( mSettings ) ) {
                        mSettings->mSock = -1;
                        Listen();
                    }
                    continue;
                }
            }
    
            // Create an entry for the connection list
            listtemp = new Iperf_ListEntry;
            memcpy(listtemp, &server->peer, sizeof(iperf_sockaddr));
            listtemp->next = NULL;
    
            // See if we need to do summing
            Mutex_Lock( &clients_mutex );
            exist = Iperf_hostpresent( &server->peer, clients); 
    
            if ( exist != NULL ) {
                // Copy group ID
                listtemp->holder = exist->holder;
                server->multihdr = exist->holder;
            } else {
                server->mThreads = 0;
                Mutex_Lock( &groupCond );
                groupID--;
                listtemp->holder = InitMulti( server, groupID );
                server->multihdr = listtemp->holder;
                Mutex_Unlock( &groupCond );
            }
    
            // Store entry in connection list
            Iperf_pushback( listtemp, &clients ); 
            Mutex_Unlock( &clients_mutex ); 
    
            tempSettings = NULL;
            if ( !isCompat( mSettings ) && !isMulticast( mSettings ) ) {
                if ( !UDP ) {
                    // TCP does not have the info yet
                    if ( recv( server->mSock, (char*)hdr, sizeof(client_hdr), 0) > 0 ) {
                        Settings_GenerateClientSettings( server, &tempSettings, 
                                                          hdr );
                    }
                } else {
                    Settings_GenerateClientSettings( server, &tempSettings, 
                                                      hdr );
                }
            }
    
    
            if ( tempSettings != NULL ) {
                client_init( tempSettings );
                if ( tempSettings->mMode == kTest_DualTest ) {
#ifdef HAVE_THREAD
                    server->runNow =  tempSettings;
#else
                    server->runNext = tempSettings;
#endif
                } else {
                    server->runNext =  tempSettings;
                }
            }
    
            // Start the server
#if defined(WIN32) && defined(HAVE_THREAD)
            if ( UDP ) {
                // WIN32 does bad UDP handling so run single threaded
                if ( server->runNow != NULL ) {
                    thread_start( server->runNow );
                }
                server_spawn( server );
                if ( server->runNext != NULL ) {
                    thread_start( server->runNext );
                }
            } else
#endif
            thread_start( server );
    
            // create a new socket
            if ( UDP ) {
                mSettings->mSock = -1; 
                Listen( );
            }
    
            // Prep for next connection
            if ( !isSingleClient( mSettings ) ) {
                mClients--;
            }
            Settings_Copy( mSettings, &server );
            server->mThreadMode = kMode_Server;
        } while ( !sInterupted && (!mCount || ( mCount && mClients > 0 )) );
    
        Settings_Destroy( server );
    }
} // end Run 

/* -------------------------------------------------------------------
 * Setup a socket listening on a port.
 * For TCP, this calls bind() and listen().
 * For UDP, this just calls bind().
 * If inLocalhost is not null, bind to that address rather than the
 * wildcard server address, specifying what incoming interface to
 * accept connections on.
 * ------------------------------------------------------------------- */
void Listener::Listen( ) {
    int rc;

    SockAddr_localAddr( mSettings );

    // create an internet TCP socket
    int type = (isUDP( mSettings )  ?  SOCK_DGRAM  :  SOCK_STREAM);
    int domain = (SockAddr_isIPv6( &mSettings->local ) ? 
#ifdef HAVE_IPV6
                  AF_INET6
#else
                  AF_INET
#endif
                  : AF_INET);

#ifdef WIN32
    if ( SockAddr_isMulticast( &mSettings->local ) ) {
        // Multicast on Win32 requires special handling
        mSettings->mSock = WSASocket( domain, type, 0, 0, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF );
        WARN_errno( mSettings->mSock == INVALID_SOCKET, "socket" );

    } else
#endif
    {
        mSettings->mSock = socket( domain, type, 0 );
        WARN_errno( mSettings->mSock == INVALID_SOCKET, "socket" );
    } 

    SetSocketOptions( mSettings );

    // reuse the address, so we can run if a former server was killed off
    int boolean = 1;
    Socklen_t len = sizeof(boolean);
    setsockopt( mSettings->mSock, SOL_SOCKET, SO_REUSEADDR, (char*) &boolean, len );

    // bind socket to server address
#ifdef WIN32
    if ( SockAddr_isMulticast( &mSettings->local ) ) {
        // Multicast on Win32 requires special handling
        rc = WSAJoinLeaf( mSettings->mSock, (sockaddr*) &mSettings->local, mSettings->size_local,0,0,0,0,JL_BOTH);
        WARN_errno( rc == SOCKET_ERROR, "WSAJoinLeaf (aka bind)" );
    } else
#endif
    {
        rc = bind( mSettings->mSock, (sockaddr*) &mSettings->local, mSettings->size_local );
        WARN_errno( rc == SOCKET_ERROR, "bind" );
    }
    // listen for connections (TCP only).
    // default backlog traditionally 5
    if ( !isUDP( mSettings ) ) {
        rc = listen( mSettings->mSock, 5 );
        WARN_errno( rc == SOCKET_ERROR, "listen" );
    }

#ifndef WIN32
    // if multicast, join the group
    if ( SockAddr_isMulticast( &mSettings->local ) ) {
        McastJoin( );
    }
#endif
} // end Listen

/* -------------------------------------------------------------------
 * Joins the multicast group, with the default interface.
 * ------------------------------------------------------------------- */

void Listener::McastJoin( ) {
#ifdef HAVE_MULTICAST
    if ( !SockAddr_isIPv6( &mSettings->local ) ) {
        struct ip_mreq mreq;

        memcpy( &mreq.imr_multiaddr, SockAddr_get_in_addr( &mSettings->local ), 
                sizeof(mreq.imr_multiaddr));

        mreq.imr_interface.s_addr = htonl( INADDR_ANY );

        int rc = setsockopt( mSettings->mSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                             (char*) &mreq, sizeof(mreq));
        WARN_errno( rc == SOCKET_ERROR, "multicast join" );
    }
#ifdef HAVE_IPV6_MULTICAST
      else {
        struct ipv6_mreq mreq;

        memcpy( &mreq.ipv6mr_multiaddr, SockAddr_get_in6_addr( &mSettings->local ), 
                sizeof(mreq.ipv6mr_multiaddr));

        mreq.ipv6mr_interface = 0;

        int rc = setsockopt( mSettings->mSock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                             (char*) &mreq, sizeof(mreq));
        WARN_errno( rc == SOCKET_ERROR, "multicast join" );
    }
#endif
#endif
}
// end McastJoin

/* -------------------------------------------------------------------
 * Sets the Multicast TTL for outgoing packets.
 * ------------------------------------------------------------------- */

void Listener::McastSetTTL( int val ) {
#ifdef HAVE_MULTICAST
    if ( !SockAddr_isIPv6( &mSettings->local ) ) {
        int rc = setsockopt( mSettings->mSock, IPPROTO_IP, IP_MULTICAST_TTL,
                             (char*) &val, sizeof(val));
        WARN_errno( rc == SOCKET_ERROR, "multicast ttl" );
    }
#ifdef HAVE_IPV6_MULTICAST
      else {
        int rc = setsockopt( mSettings->mSock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                             (char*) &val, sizeof(val));
        WARN_errno( rc == SOCKET_ERROR, "multicast ttl" );
    }
#endif
#endif
}
// end McastSetTTL

/* -------------------------------------------------------------------
 * After Listen() has setup mSock, this will block
 * until a new connection arrives.
 * ------------------------------------------------------------------- */

void Listener::Accept( thread_Settings *server ) {

    server->size_peer = sizeof(iperf_sockaddr); 
    if ( isUDP( server ) ) {
        /* ------------------------------------------------------------------- 
         * Do the equivalent of an accept() call for UDP sockets. This waits 
         * on a listening UDP socket until we get a datagram. 
         * ------------------------------------------------------------------- */
        int rc;
        Iperf_ListEntry *exist;
        int32_t datagramID;
        server->mSock = INVALID_SOCKET;
        while ( server->mSock == INVALID_SOCKET ) {
            rc = recvfrom( mSettings->mSock, mBuf, mSettings->mBufLen, 0, 
                           (struct sockaddr*) &server->peer, &server->size_peer );
            FAIL_errno( rc == SOCKET_ERROR, "recvfrom", mSettings );

            Mutex_Lock( &clients_mutex );
    
            // Handle connection for UDP sockets.
            exist = Iperf_present( &server->peer, clients);
            datagramID = ntohl( ((UDP_datagram*) mBuf)->id ); 
            if ( exist == NULL && datagramID >= 0 ) {
                server->mSock = mSettings->mSock;
                int rc = connect( server->mSock, (struct sockaddr*) &server->peer,
                                  server->size_peer );
                FAIL_errno( rc == SOCKET_ERROR, "connect UDP", mSettings );
            } else {
                server->mSock = INVALID_SOCKET;
            }
            Mutex_Unlock( &clients_mutex );
        }
    } else {
        // Handles interupted accepts. Returns the newly connected socket.
        server->mSock = INVALID_SOCKET;
    
        while ( server->mSock == INVALID_SOCKET ) {
            // accept a connection
            server->mSock = accept( mSettings->mSock, 
                                    (sockaddr*) &server->peer, &server->size_peer );
            if ( server->mSock == INVALID_SOCKET &&  errno == EINTR ) {
                continue;
            }
        }
    }
    server->size_local = sizeof(iperf_sockaddr); 
    getsockname( server->mSock, (sockaddr*) &server->local, 
                 &server->size_local );
} // end Accept

void Listener::UDPSingleServer( ) {
    
    bool client = false, UDP = isUDP( mSettings ), mCount = (mSettings->mThreads != 0);
    thread_Settings *tempSettings = NULL;
    Iperf_ListEntry *exist, *listtemp;
    int rc;
    int32_t datagramID;
    client_hdr* hdr = ( UDP ? (client_hdr*) (((UDP_datagram*)mBuf) + 1) : 
                              (client_hdr*) mBuf);
    ReportStruct *reportstruct = new ReportStruct;
    
    if ( mSettings->mHost != NULL ) {
        client = true;
        SockAddr_remoteAddr( mSettings );
    }
    Settings_Copy( mSettings, &server );
    server->mThreadMode = kMode_Server;


    // Accept each packet, 
    // If there is no existing client, then start  
    // a new report to service the new client 
    // The listener runs in a single thread 
    Mutex_Lock( &clients_mutex );
    do {
        // Get next packet
        while ( sInterupted == 0) {
            server->size_peer = sizeof( iperf_sockaddr );
            rc = recvfrom( mSettings->mSock, mBuf, mSettings->mBufLen, 0, 
                           (struct sockaddr*) &server->peer, &server->size_peer );
            WARN_errno( rc == SOCKET_ERROR, "recvfrom" );
            if ( rc == SOCKET_ERROR ) {
                return;
            }
        
        
            // Handle connection for UDP sockets.
            exist = Iperf_present( &server->peer, clients);
            datagramID = ntohl( ((UDP_datagram*) mBuf)->id ); 
            if ( datagramID >= 0 ) {
                if ( exist != NULL ) {
                    // read the datagram ID and sentTime out of the buffer 
                    reportstruct->packetID = datagramID; 
                    reportstruct->sentTime.tv_sec = ntohl( ((UDP_datagram*) mBuf)->tv_sec  );
                    reportstruct->sentTime.tv_usec = ntohl( ((UDP_datagram*) mBuf)->tv_usec ); 
        
                    reportstruct->packetLen = rc;
                    gettimeofday( &(reportstruct->packetTime), NULL );
        
                    ReportPacket( exist->server->reporthdr, reportstruct );
                } else {
                    Mutex_Lock( &groupCond );
                    groupID--;
                    server->mSock = -groupID;
                    Mutex_Unlock( &groupCond );
                    server->size_local = sizeof(iperf_sockaddr); 
                    getsockname( mSettings->mSock, (sockaddr*) &server->local, 
                                 &server->size_local );
                    break;
                }
            } else {
                if ( exist != NULL ) {
                    // read the datagram ID and sentTime out of the buffer 
                    reportstruct->packetID = -datagramID; 
                    reportstruct->sentTime.tv_sec = ntohl( ((UDP_datagram*) mBuf)->tv_sec  );
                    reportstruct->sentTime.tv_usec = ntohl( ((UDP_datagram*) mBuf)->tv_usec ); 
        
                    reportstruct->packetLen = rc;
                    gettimeofday( &(reportstruct->packetTime), NULL );
        
                    ReportPacket( exist->server->reporthdr, reportstruct );
                    // stop timing 
                    gettimeofday( &(reportstruct->packetTime), NULL );
                    CloseReport( exist->server->reporthdr, reportstruct );
        
                    if ( rc > (int) ( sizeof( UDP_datagram )
                                                      + sizeof( server_hdr ) ) ) {
                        UDP_datagram *UDP_Hdr;
                        server_hdr *hdr;
        
                        UDP_Hdr = (UDP_datagram*) mBuf;
                        Transfer_Info *stats = GetReport( exist->server->reporthdr );
                        hdr = (server_hdr*) (UDP_Hdr+1);
        
                        hdr->flags        = htonl( HEADER_VERSION1 );
                        hdr->total_len1   = htonl( (long) (stats->TotalLen >> 32) );
                        hdr->total_len2   = htonl( (long) (stats->TotalLen & 0xFFFFFFFF) );
                        hdr->stop_sec     = htonl( (long) stats->endTime );
                        hdr->stop_usec    = htonl( (long)((stats->endTime - (long)stats->endTime)
                                                          * rMillion));
                        hdr->error_cnt    = htonl( stats->cntError );
                        hdr->outorder_cnt = htonl( stats->cntOutofOrder );
                        hdr->datagrams    = htonl( stats->cntDatagrams );
                        hdr->jitter1      = htonl( (long) stats->jitter );
                        hdr->jitter2      = htonl( (long) ((stats->jitter - (long)stats->jitter) 
                                                           * rMillion) );
        
                    }
                    EndReport( exist->server->reporthdr );
                    exist->server->reporthdr = NULL;
                    Iperf_delete( &(exist->server->peer), &clients );
                } else if ( rc > (int) ( sizeof( UDP_datagram )
                                                  + sizeof( server_hdr ) ) ) {
                    UDP_datagram *UDP_Hdr;
                    server_hdr *hdr;
        
                    UDP_Hdr = (UDP_datagram*) mBuf;
                    hdr = (server_hdr*) (UDP_Hdr+1);
                    hdr->flags = htonl( 0 );
                }
                sendto( mSettings->mSock, mBuf, mSettings->mBufLen, 0,
                        (struct sockaddr*) &server->peer, server->size_peer);
            }
        }
        if ( server->mSock == INVALID_SOCKET ) {
            break;
        }
        if ( sInterupted != 0 ) {
            close( server->mSock );
            break;
        }
        // Reset Single Client Stuff
        if ( isSingleClient( mSettings ) && clients == NULL ) {
            mSettings->peer = server->peer;
            mClients--;
            client = true;
            // Once all the server threads exit then quit
            // Must keep going in case this client sends
            // more streams
            if ( mClients == 0 ) {
                thread_release_nonterm( 0 );
                mClients = 1;
            }
        }
        // Verify that it is allowed
        if ( client ) {
            if ( !SockAddr_Hostare_Equal( (sockaddr*) &mSettings->peer, 
                                          (sockaddr*) &server->peer ) ) {
                // Not allowed try again
                connect( mSettings->mSock, 
                         (sockaddr*) &server->peer, 
                         server->size_peer );
                close( mSettings->mSock );
                mSettings->mSock = -1; 
                Listen( );
                continue;
            }
        }

        // Create an entry for the connection list
        listtemp = new Iperf_ListEntry;
        memcpy(listtemp, &server->peer, sizeof(iperf_sockaddr));
        listtemp->server = server;
        listtemp->next = NULL;

        // See if we need to do summing
        exist = Iperf_hostpresent( &server->peer, clients); 

        if ( exist != NULL ) {
            // Copy group ID
            listtemp->holder = exist->holder;
            server->multihdr = exist->holder;
        } else {
            server->mThreads = 0;
            Mutex_Lock( &groupCond );
            groupID--;
            listtemp->holder = InitMulti( server, groupID );
            server->multihdr = listtemp->holder;
            Mutex_Unlock( &groupCond );
        }

        // Store entry in connection list
        Iperf_pushback( listtemp, &clients ); 

        tempSettings = NULL;
        if ( !isCompat( mSettings ) && !isMulticast( mSettings ) ) {
            Settings_GenerateClientSettings( server, &tempSettings, 
                                              hdr );
        }


        if ( tempSettings != NULL ) {
            client_init( tempSettings );
            if ( tempSettings->mMode == kTest_DualTest ) {
#ifdef HAVE_THREAD
                thread_start( tempSettings );
#else
                server->runNext = tempSettings;
#endif
            } else {
                server->runNext =  tempSettings;
            }
        }
        server->reporthdr = InitReport( server );

        // Prep for next connection
        if ( !isSingleClient( mSettings ) ) {
            mClients--;
        }
        Settings_Copy( mSettings, &server );
        server->mThreadMode = kMode_Server;
    } while ( !sInterupted && (!mCount || ( mCount && mClients > 0 )) );
    Mutex_Unlock( &clients_mutex );

    Settings_Destroy( server );
}

/* -------------------------------------------------------------------- 
 * Run the server as a daemon  
 * --------------------------------------------------------------------*/ 

void Listener::runAsDaemon(const char *pname, int facility) {
#ifndef WIN32 
    pid_t pid; 

    /* Create a child process & if successful, exit from the parent process */ 
    if ( (pid = fork()) == -1 ) {
        fprintf( stderr, "error in first child create\n");     
        exit(0); 
    } else if ( pid != 0 ) {
        exit(0); 
    }

    /* Try becoming the session leader, once the parent exits */
    if ( setsid() == -1 ) {           /* Become the session leader */ 
        fprintf( stderr, "Cannot change the session group leader\n"); 
    } else {
    } 
    signal(SIGHUP,SIG_IGN); 


    /* Now fork() and get released from the terminal */  
    if ( (pid = fork()) == -1 ) {
        fprintf( stderr, "error\n");   
        exit(0); 
    } else if ( pid != 0 ) {
        exit(0); 
    }

    chdir("."); 
    fprintf( stderr, "Running Iperf Server as a daemon\n"); 
    fprintf( stderr, "The Iperf daemon process ID : %d\n",((int)getpid())); 
    fflush(stderr); 

    fclose(stdin); 
#else 
    fprintf( stderr, "Use the precompiled windows version for service (daemon) option\n"); 
#endif  

}

