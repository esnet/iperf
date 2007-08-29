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
 * Launch.cpp
 * by Kevin Gibbs <kgibbs@nlanr.net> 
 * ------------------------------------------------------------------- 
 * Functions to launch new server and client threads from C while
 * the server and client are in C++.
 * The launch function for reporters is in Reporter.c since it is
 * in C and does not need a special launching function.
 * ------------------------------------------------------------------- */ 

#include "headers.h"
#include "Thread.h"
#include "Settings.hpp"
#include "Client.hpp"
#include "Listener.hpp"
#include "Server.hpp"
#include "PerfSocket.hpp"

/*
 * listener_spawn is responsible for creating a Listener class
 * and launching the listener. It is provided as a means for
 * the C thread subsystem to launch the listener C++ object.
 */
void listener_spawn( thread_Settings *thread ) {
    Listener *theListener = NULL;

    // start up a listener
    theListener = new Listener( thread );
#ifndef WIN32
    // handling of daemon mode in non-win32 builds
    if ( isDaemon( thread ) ) {
        theListener->runAsDaemon("iperf",LOG_DAEMON);
    }
#endif

    // Start listening
    theListener->Run();
    DELETE_PTR( theListener );
}

/*
 * server_spawn is responsible for creating a Server class
 * and launching the server. It is provided as a means for
 * the C thread subsystem to launch the server C++ object.
 */
void server_spawn( thread_Settings *thread) {
    Server *theServer = NULL;

    // Start up the server
    theServer = new Server( thread );
    
    // Run the test
    theServer->Run();
    DELETE_PTR( theServer);
}

/*
 * client_spawn is responsible for creating a Client class
 * and launching the client. It is provided as a means for
 * the C thread subsystem to launch the client C++ object.
 */
void client_spawn( thread_Settings *thread ) {
    Client *theClient = NULL;

    //start up the client
    theClient = new Client( thread );

    // Let the server know about our settings
    theClient->InitiateServer();

    // Run the test
    theClient->Run();
    DELETE_PTR( theClient );
}

/*
 * client_init handles multiple threaded connects. It creates
 * a listener object if either the dual test or tradeoff were
 * specified. It also creates settings structures for all the
 * threads and arranges them so they can be managed and started
 * via the one settings structure that was passed in.
 */
void client_init( thread_Settings *clients ) {
    thread_Settings *itr = NULL;
    thread_Settings *next = NULL;

    // Set the first thread to report Settings
    setReport( clients );
    itr = clients;

    // See if we need to start a listener as well
    Settings_GenerateListenerSettings( clients, &next );

    // Create a multiple report header to handle reporting the
    // sum of multiple client threads
    Mutex_Lock( &groupCond );
    groupID--;
    clients->multihdr = InitMulti( clients, groupID );
    Mutex_Unlock( &groupCond );

#ifdef HAVE_THREAD
    if ( next != NULL ) {
        // We have threads and we need to start a listener so
        // have it ran before the client is launched
        itr->runNow = next;
        itr = next;
    }
#endif
    // For each of the needed threads create a copy of the
    // provided settings, unsetting the report flag and add
    // to the list of threads to start
    for (int i = 1; i < clients->mThreads; i++) {
        Settings_Copy( clients, &next );
        unsetReport( next );
        itr->runNow = next;
        itr = next;
    }
#ifndef HAVE_THREAD
    if ( next != NULL ) {
        // We don't have threads and we need to start a listener so
        // have it ran after the client is finished
        itr->runNext = next;
    }
#endif
}

