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
 * Settings.cpp
 * by Mark Gates <mgates@nlanr.net>
 * & Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * -------------------------------------------------------------------
 * Stores and parses the initial values for all the global variables.
 * -------------------------------------------------------------------
 * headers
 * uses
 *   <stdlib.h>
 *   <stdio.h>
 *   <string.h>
 *
 *   <unistd.h>
 * ------------------------------------------------------------------- */

#define HEADERS()

#include "headers.h"

#include "Settings.hpp"
#include "Locale.h"
#include "SocketAddr.h"

#include "util.h"

#include "gnu_getopt.h"

void Settings_Interpret( char option, const char *optarg, thread_Settings *mExtSettings );

/* -------------------------------------------------------------------
 * command line options
 *
 * The option struct essentially maps a long option name (--foobar)
 * or environment variable ($FOOBAR) to its short option char (f).
 * ------------------------------------------------------------------- */
#define LONG_OPTIONS()

const struct option long_options[] =
{
{"singleclient",     no_argument, NULL, '1'},
{"bandwidth",  required_argument, NULL, 'b'},
{"client",     required_argument, NULL, 'c'},
{"dualtest",         no_argument, NULL, 'd'},
{"format",     required_argument, NULL, 'f'},
{"help",             no_argument, NULL, 'h'},
{"interval",   required_argument, NULL, 'i'},
{"len",        required_argument, NULL, 'l'},
{"print_mss",        no_argument, NULL, 'm'},
{"num",        required_argument, NULL, 'n'},
{"output",     required_argument, NULL, 'o'},
{"port",       required_argument, NULL, 'p'},
{"tradeoff",         no_argument, NULL, 'r'},
{"server",           no_argument, NULL, 's'},
{"time",       required_argument, NULL, 't'},
{"udp",              no_argument, NULL, 'u'},
{"version",          no_argument, NULL, 'v'},
{"window",     required_argument, NULL, 'w'},
{"reportexclude", required_argument, NULL, 'x'},
{"reportstyle",required_argument, NULL, 'y'},

// more esoteric options
{"bind",       required_argument, NULL, 'B'},
{"compatibility",    no_argument, NULL, 'C'},
{"daemon",           no_argument, NULL, 'D'},
{"file_input", required_argument, NULL, 'F'},
{"stdin_input",      no_argument, NULL, 'I'},
{"mss",        required_argument, NULL, 'M'},
{"nodelay",          no_argument, NULL, 'N'},
{"listenport", required_argument, NULL, 'L'},
{"parallel",   required_argument, NULL, 'P'},
{"remove",           no_argument, NULL, 'R'},
{"tos",        required_argument, NULL, 'S'},
{"ttl",        required_argument, NULL, 'T'},
{"single_udp",       no_argument, NULL, 'U'},
{"ipv6_domain",      no_argument, NULL, 'V'},
{"suggest_win_size", no_argument, NULL, 'W'},
{"linux-congestion", required_argument, NULL, 'Z'},
{0, 0, 0, 0}
};

#define ENV_OPTIONS()

const struct option env_options[] =
{
{"IPERF_SINGLECLIENT",     no_argument, NULL, '1'},
{"IPERF_BANDWIDTH",  required_argument, NULL, 'b'},
{"IPERF_CLIENT",     required_argument, NULL, 'c'},
{"IPERF_DUALTEST",         no_argument, NULL, 'd'},
{"IPERF_FORMAT",     required_argument, NULL, 'f'},
// skip help
{"IPERF_INTERVAL",   required_argument, NULL, 'i'},
{"IPERF_LEN",        required_argument, NULL, 'l'},
{"IPERF_PRINT_MSS",        no_argument, NULL, 'm'},
{"IPERF_NUM",        required_argument, NULL, 'n'},
{"IPERF_PORT",       required_argument, NULL, 'p'},
{"IPERF_TRADEOFF",         no_argument, NULL, 'r'},
{"IPERF_SERVER",           no_argument, NULL, 's'},
{"IPERF_TIME",       required_argument, NULL, 't'},
{"IPERF_UDP",              no_argument, NULL, 'u'},
// skip version
{"TCP_WINDOW_SIZE",  required_argument, NULL, 'w'},
{"IPERF_REPORTEXCLUDE", required_argument, NULL, 'x'},
{"IPERF_REPORTSTYLE",required_argument, NULL, 'y'},

// more esoteric options
{"IPERF_BIND",       required_argument, NULL, 'B'},
{"IPERF_COMPAT",           no_argument, NULL, 'C'},
{"IPERF_DAEMON",           no_argument, NULL, 'D'},
{"IPERF_FILE_INPUT", required_argument, NULL, 'F'},
{"IPERF_STDIN_INPUT",      no_argument, NULL, 'I'},
{"IPERF_MSS",        required_argument, NULL, 'M'},
{"IPERF_NODELAY",          no_argument, NULL, 'N'},
{"IPERF_LISTENPORT", required_argument, NULL, 'L'},
{"IPERF_PARALLEL",   required_argument, NULL, 'P'},
{"IPERF_TOS",        required_argument, NULL, 'S'},
{"IPERF_TTL",        required_argument, NULL, 'T'},
{"IPERF_SINGLE_UDP",       no_argument, NULL, 'U'},
{"IPERF_IPV6_DOMAIN",      no_argument, NULL, 'V'},
{"IPERF_SUGGEST_WIN_SIZE", required_argument, NULL, 'W'},
{"IPERF_CONGESTION_CONTROL",  required_argument, NULL, 'Z'},
{0, 0, 0, 0}
};

#define SHORT_OPTIONS()

const char short_options[] = "1b:c:df:hi:l:mn:o:p:rst:uvw:x:y:B:CDF:IL:M:NP:RS:T:UVWZ:";

/* -------------------------------------------------------------------
 * defaults
 * ------------------------------------------------------------------- */
#define DEFAULTS()

const long kDefault_UDPRate = 1024 * 1024; // -u  if set, 1 Mbit/sec
const int  kDefault_UDPBufLen = 1470;      // -u  if set, read/write 1470 bytes
// 1470 bytes is small enough to be sending one packet per datagram on ethernet

// 1450 bytes is small enough to be sending one packet per datagram on ethernet
//  **** with IPv6 ****

/* -------------------------------------------------------------------
 * Initialize all settings to defaults.
 * ------------------------------------------------------------------- */

void Settings_Initialize( thread_Settings *main ) {
    // Everything defaults to zero or NULL with
    // this memset. Only need to set non-zero values
    // below.
    memset( main, 0, sizeof(thread_Settings) );
    main->mSock = INVALID_SOCKET;
    main->mReportMode = kReport_Default;
    // option, defaults
    main->flags         = FLAG_MODETIME | FLAG_STDOUT; // Default time and stdout
    //main->mUDPRate      = 0;           // -b,  ie. TCP mode
    //main->mHost         = NULL;        // -c,  none, required for client
    main->mMode         = kTest_Normal;  // -d,  mMode == kTest_DualTest
    main->mFormat       = 'a';           // -f,  adaptive bits
    // skip help                         // -h,
    //main->mBufLenSet  = false;         // -l,	
    main->mBufLen       = 8 * 1024;      // -l,  8 Kbyte
    //main->mInterval     = 0;           // -i,  ie. no periodic bw reports
    //main->mPrintMSS   = false;         // -m,  don't print MSS
    // mAmount is time also              // -n,  N/A
    //main->mOutputFileName = NULL;      // -o,  filename
    main->mPort         = 5001;          // -p,  ttcp port
    // mMode    = kTest_Normal;          // -r,  mMode == kTest_TradeOff
    main->mThreadMode   = kMode_Unknown; // -s,  or -c, none
    main->mAmount       = 1000;          // -t,  10 seconds
    // mUDPRate > 0 means UDP            // -u,  N/A, see kDefault_UDPRate
    // skip version                      // -v,
    //main->mTCPWin       = 0;           // -w,  ie. don't set window

    // more esoteric options
    //main->mLocalhost    = NULL;        // -B,  none
    //main->mCompat     = false;         // -C,  run in Compatibility mode
    //main->mDaemon     = false;         // -D,  run as a daemon
    //main->mFileInput  = false;         // -F,
    //main->mFileName     = NULL;        // -F,  filename 
    //main->mStdin      = false;         // -I,  default not stdin
    //main->mListenPort   = 0;           // -L,  listen port
    //main->mMSS          = 0;           // -M,  ie. don't set MSS
    //main->mNodelay    = false;         // -N,  don't set nodelay
    //main->mThreads      = 0;           // -P,
    //main->mRemoveService = false;      // -R,
    //main->mTOS          = 0;           // -S,  ie. don't set type of service
    main->mTTL          = 1;             // -T,  link-local TTL
    //main->mDomain     = kMode_IPv4;    // -V,
    //main->mSuggestWin = false;         // -W,  Suggest the window size.

} // end Settings

void Settings_Copy( thread_Settings *from, thread_Settings **into ) {
    *into = new thread_Settings;
    memcpy( *into, from, sizeof(thread_Settings) );
    if ( from->mHost != NULL ) {
        (*into)->mHost = new char[ strlen(from->mHost) + 1];
        strcpy( (*into)->mHost, from->mHost );
    }
    if ( from->mOutputFileName != NULL ) {
        (*into)->mOutputFileName = new char[ strlen(from->mOutputFileName) + 1];
        strcpy( (*into)->mOutputFileName, from->mOutputFileName );
    }
    if ( from->mLocalhost != NULL ) {
        (*into)->mLocalhost = new char[ strlen(from->mLocalhost) + 1];
        strcpy( (*into)->mLocalhost, from->mLocalhost );
    }
    if ( from->mFileName != NULL ) {
        (*into)->mFileName = new char[ strlen(from->mFileName) + 1];
        strcpy( (*into)->mFileName, from->mFileName );
    }
    // Zero out certain entries
    (*into)->mTID = thread_zeroid();
    (*into)->runNext = NULL;
    (*into)->runNow = NULL;
}

/* -------------------------------------------------------------------
 * Delete memory: Does not clean up open file pointers or ptr_parents
 * ------------------------------------------------------------------- */

void Settings_Destroy( thread_Settings *mSettings) {
    DELETE_ARRAY( mSettings->mHost      );
    DELETE_ARRAY( mSettings->mLocalhost );
    DELETE_ARRAY( mSettings->mFileName  );
    DELETE_ARRAY( mSettings->mOutputFileName );
    DELETE_PTR( mSettings );
} // end ~Settings

/* -------------------------------------------------------------------
 * Parses settings from user's environment variables.
 * ------------------------------------------------------------------- */
void Settings_ParseEnvironment( thread_Settings *mSettings ) {
    char *theVariable;

    int i = 0;
    while ( env_options[i].name != NULL ) {
        theVariable = getenv( env_options[i].name );
        if ( theVariable != NULL ) {
            Settings_Interpret( env_options[i].val, theVariable, mSettings );
        }
        i++;
    }
} // end ParseEnvironment

/* -------------------------------------------------------------------
 * Parse settings from app's command line.
 * ------------------------------------------------------------------- */

void Settings_ParseCommandLine( int argc, char **argv, thread_Settings *mSettings ) {
    int option;
    while ( (option =
             gnu_getopt_long( argc, argv, short_options,
                              long_options, NULL )) != EOF ) {
        Settings_Interpret( option, gnu_optarg, mSettings );
    }

    for ( int i = gnu_optind; i < argc; i++ ) {
        fprintf( stderr, "%s: ignoring extra argument -- %s\n", argv[0], argv[i] );
    }
} // end ParseCommandLine

/* -------------------------------------------------------------------
 * Interpret individual options, either from the command line
 * or from environment variables.
 * ------------------------------------------------------------------- */

void Settings_Interpret( char option, const char *optarg, thread_Settings *mExtSettings ) {
    char outarg[100];

    switch ( option ) {
        case '1': // Single Client
            setSingleClient( mExtSettings );
            break;
        case 'b': // UDP bandwidth
            if ( !isUDP( mExtSettings ) ) {
                fprintf( stderr, warn_implied_udp, option );
            }

            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }

            Settings_GetLowerCaseArg(optarg,outarg);
            mExtSettings->mUDPRate = byte_atoi(outarg);
            setUDP( mExtSettings );

            // if -l has already been processed, mBufLenSet is true
            // so don't overwrite that value.
            if ( !isBuflenSet( mExtSettings ) ) {
                mExtSettings->mBufLen = kDefault_UDPBufLen;
            }
            break;

        case 'c': // client mode w/ server host to connect to
            mExtSettings->mHost = new char[ strlen( optarg ) + 1 ];
            strcpy( mExtSettings->mHost, optarg );

            if ( mExtSettings->mThreadMode == kMode_Unknown ) {
                // Test for Multicast
                iperf_sockaddr temp;
                SockAddr_setHostname( mExtSettings->mHost, &temp,
                                      (isIPV6( mExtSettings ) ? 1 : 0 ));
                if ( SockAddr_isMulticast( &temp ) ) {
                    setMulticast( mExtSettings );
                }
                mExtSettings->mThreadMode = kMode_Client;
                mExtSettings->mThreads = 1;
            }
            break;

        case 'd': // Dual-test Mode
            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }
            if ( isCompat( mExtSettings ) ) {
                fprintf( stderr, warn_invalid_compatibility_option, option );
            }
#ifdef HAVE_THREAD
            mExtSettings->mMode = kTest_DualTest;
#else
            fprintf( stderr, warn_invalid_single_threaded, option );
            mExtSettings->mMode = kTest_TradeOff;
#endif
            break;

        case 'f': // format to print in
            mExtSettings->mFormat = (*optarg);
            break;

        case 'h': // print help and exit
            fprintf(stderr, usage_long1);
            fprintf(stderr, usage_long2);
            exit(1);
            break;

        case 'i': // specify interval between periodic bw reports
            mExtSettings->mInterval = atof( optarg );
            if ( mExtSettings->mInterval < 0.5 ) {
                fprintf (stderr, report_interval_small, mExtSettings->mInterval);
                mExtSettings->mInterval = 0.5;
            }
            break;

        case 'l': // length of each buffer
            Settings_GetUpperCaseArg(optarg,outarg);
            mExtSettings->mBufLen = byte_atoi( outarg );
            setBuflenSet( mExtSettings );
            if ( !isUDP( mExtSettings ) ) {
                 if ( mExtSettings->mBufLen < (int) sizeof( client_hdr ) &&
                      !isCompat( mExtSettings ) ) {
                    setCompat( mExtSettings );
                    fprintf( stderr, warn_implied_compatibility, option );
                 }
            } else {
                if ( mExtSettings->mBufLen < (int) sizeof( UDP_datagram ) ) {
                    mExtSettings->mBufLen = sizeof( UDP_datagram );
                    fprintf( stderr, warn_buffer_too_small, mExtSettings->mBufLen );
                }
                if ( !isCompat( mExtSettings ) &&
                            mExtSettings->mBufLen < (int) ( sizeof( UDP_datagram )
                            + sizeof( client_hdr ) ) ) {
                    setCompat( mExtSettings );
                    fprintf( stderr, warn_implied_compatibility, option );
                }
            }

            break;

        case 'm': // print TCP MSS
            setPrintMSS( mExtSettings );
            break;

        case 'n': // bytes of data
            // amount mode (instead of time mode)
            unsetModeTime( mExtSettings );
            Settings_GetUpperCaseArg(optarg,outarg);
            mExtSettings->mAmount = byte_atoi( outarg );
            break;

        case 'o' : // output the report and other messages into the file
            unsetSTDOUT( mExtSettings );
            mExtSettings->mOutputFileName = new char[strlen(optarg)+1];
            strcpy( mExtSettings->mOutputFileName, optarg);
            break;

        case 'p': // server port
            mExtSettings->mPort = atoi( optarg );
            break;

        case 'r': // test mode tradeoff
            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }
            if ( isCompat( mExtSettings ) ) {
                fprintf( stderr, warn_invalid_compatibility_option, option );
            }

            mExtSettings->mMode = kTest_TradeOff;
            break;

        case 's': // server mode
            if ( mExtSettings->mThreadMode != kMode_Unknown ) {
                fprintf( stderr, warn_invalid_client_option, option );
                break;
            }

            mExtSettings->mThreadMode = kMode_Listener;
            break;

        case 't': // seconds to write for
            // time mode (instead of amount mode)
            setModeTime( mExtSettings );
            mExtSettings->mAmount = (int) (atof( optarg ) * 100.0);
            break;

        case 'u': // UDP instead of TCP
            // if -b has already been processed, UDP rate will
            // already be non-zero, so don't overwrite that value
            if ( !isUDP( mExtSettings ) ) {
                setUDP( mExtSettings );
                mExtSettings->mUDPRate = kDefault_UDPRate;
            }

            // if -l has already been processed, mBufLenSet is true
            // so don't overwrite that value.
            if ( !isBuflenSet( mExtSettings ) ) {
                mExtSettings->mBufLen = kDefault_UDPBufLen;
            } else if ( mExtSettings->mBufLen < (int) ( sizeof( UDP_datagram ) 
                        + sizeof( client_hdr ) ) &&
                        !isCompat( mExtSettings ) ) {
                setCompat( mExtSettings );
                fprintf( stderr, warn_implied_compatibility, option );
            }
            break;

        case 'v': // print version and exit
            fprintf( stderr, version );
            exit(1);
            break;

        case 'w': // TCP window size (socket buffer size)
            Settings_GetUpperCaseArg(optarg,outarg);
            mExtSettings->mTCPWin = byte_atoi(outarg);

            if ( mExtSettings->mTCPWin < 2048 ) {
                fprintf( stderr, warn_window_small, mExtSettings->mTCPWin );
            }
            break;

        case 'x': // Limit Reports
            while ( *optarg != '\0' ) {
                switch ( *optarg ) {
                    case 's':
                    case 'S':
                        setNoSettReport( mExtSettings );
                        break;
                    case 'c':
                    case 'C':
                        setNoConnReport( mExtSettings );
                        break;
                    case 'd':
                    case 'D':
                        setNoDataReport( mExtSettings );
                        break;
                    case 'v':
                    case 'V':
                        setNoServReport( mExtSettings );
                        break;
                    case 'm':
                    case 'M':
                        setNoMultReport( mExtSettings );
                        break;
                    default:
                        fprintf(stderr, warn_invalid_report, *optarg);
                }
                optarg++;
            }
            break;

        case 'y': // Reporting Style
            switch ( *optarg ) {
                case 'c':
                case 'C':
                    mExtSettings->mReportMode = kReport_CSV;
                    break;
                default:
                    fprintf( stderr, warn_invalid_report_style, optarg );
            }
            break;


            // more esoteric options
        case 'B': // specify bind address
            mExtSettings->mLocalhost = new char[ strlen( optarg ) + 1 ];
            strcpy( mExtSettings->mLocalhost, optarg );
            // Test for Multicast
            iperf_sockaddr temp;
            SockAddr_setHostname( mExtSettings->mLocalhost, &temp,
                                  (isIPV6( mExtSettings ) ? 1 : 0 ));
            if ( SockAddr_isMulticast( &temp ) ) {
                setMulticast( mExtSettings );
            }
            break;

        case 'C': // Run in Compatibility Mode
            setCompat( mExtSettings );
            if ( mExtSettings->mMode != kTest_Normal ) {
                fprintf( stderr, warn_invalid_compatibility_option,
                        ( mExtSettings->mMode == kTest_DualTest ?
                          'd' : 'r' ) );
                mExtSettings->mMode = kTest_Normal;
            }
            break;

        case 'D': // Run as a daemon
            setDaemon( mExtSettings );
            break;

        case 'F' : // Get the input for the data stream from a file
            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }

            setFileInput( mExtSettings );
            mExtSettings->mFileName = new char[strlen(optarg)+1];
            strcpy( mExtSettings->mFileName, optarg);
            break;

        case 'I' : // Set the stdin as the input source
            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }

            setFileInput( mExtSettings );
            setSTDIN( mExtSettings );
            mExtSettings->mFileName = new char[strlen("<stdin>")+1];
            strcpy( mExtSettings->mFileName,"<stdin>");
            break;

        case 'L': // Listen Port (bidirectional testing client-side)
            if ( mExtSettings->mThreadMode != kMode_Client ) {
                fprintf( stderr, warn_invalid_server_option, option );
                break;
            }

            mExtSettings->mListenPort = atoi( optarg );
            break;

        case 'M': // specify TCP MSS (maximum segment size)
            Settings_GetUpperCaseArg(optarg,outarg);

            mExtSettings->mMSS = byte_atoi( outarg );
            break;

        case 'N': // specify TCP nodelay option (disable Jacobson's Algorithm)
            setNoDelay( mExtSettings );
            break;

        case 'P': // number of client threads
#ifdef HAVE_THREAD
            mExtSettings->mThreads = atoi( optarg );
#else
            if ( mExtSettings->mThreadMode != kMode_Server ) {
                fprintf( stderr, warn_invalid_single_threaded, option );
            } else {
                mExtSettings->mThreads = atoi( optarg );
            }
#endif
            break;

        case 'R':
            setRemoveService( mExtSettings );
            break;

        case 'S': // IP type-of-service
            // TODO use a function that understands base-2
            // the zero base here allows the user to specify
            // "0x#" hex, "0#" octal, and "#" decimal numbers
            mExtSettings->mTOS = strtol( optarg, NULL, 0 );
            break;

        case 'T': // time-to-live for multicast
            mExtSettings->mTTL = atoi( optarg );
            break;

        case 'U': // single threaded UDP server
            setSingleUDP( mExtSettings );
            break;

        case 'V': // IPv6 Domain
            setIPV6( mExtSettings );
            if ( mExtSettings->mThreadMode == kMode_Server 
                 && mExtSettings->mLocalhost != NULL ) {
                // Test for Multicast
                iperf_sockaddr temp;
                SockAddr_setHostname( mExtSettings->mLocalhost, &temp, 1);
                if ( SockAddr_isMulticast( &temp ) ) {
                    setMulticast( mExtSettings );
                }
            } else if ( mExtSettings->mThreadMode == kMode_Client ) {
                // Test for Multicast
                iperf_sockaddr temp;
                SockAddr_setHostname( mExtSettings->mHost, &temp, 1 );
                if ( SockAddr_isMulticast( &temp ) ) {
                    setMulticast( mExtSettings );
                }
            }
            break;

        case 'W' :
            setSuggestWin( mExtSettings );
            fprintf( stderr, "The -W option is not available in this release\n");
            break;

        case 'Z':
#ifdef TCP_CONGESTION
	    setCongestionControl( mExtSettings );
	    mExtSettings->mCongestion = new char[strlen(optarg)+1];
	    strcpy( mExtSettings->mCongestion, optarg);
#else
            fprintf( stderr, "The -Z option is not available on this operating system\n");
#endif
	    break;

        default: // ignore unknown
            break;
    }
} // end Interpret

void Settings_GetUpperCaseArg(const char *inarg, char *outarg) {

    int len = strlen(inarg);
    strcpy(outarg,inarg);

    if ( (len > 0) && (inarg[len-1] >='a') 
         && (inarg[len-1] <= 'z') )
        outarg[len-1]= outarg[len-1]+'A'-'a';
}

void Settings_GetLowerCaseArg(const char *inarg, char *outarg) {

    int len = strlen(inarg);
    strcpy(outarg,inarg);

    if ( (len > 0) && (inarg[len-1] >='A') 
         && (inarg[len-1] <= 'Z') )
        outarg[len-1]= outarg[len-1]-'A'+'a';
}

/*
 * Settings_GenerateListenerSettings
 * Called to generate the settings to be passed to the Listener
 * instance that will handle dual testings from the client side
 * this should only return an instance if it was called on 
 * the thread_Settings instance generated from the command line 
 * for client side execution 
 */
void Settings_GenerateListenerSettings( thread_Settings *client, thread_Settings **listener ) {
    if ( !isCompat( client ) && 
         (client->mMode == kTest_DualTest || client->mMode == kTest_TradeOff) ) {
        *listener = new thread_Settings;
        memcpy(*listener, client, sizeof( thread_Settings ));
        setCompat( (*listener) );
        unsetDaemon( (*listener) );
        if ( client->mListenPort != 0 ) {
            (*listener)->mPort   = client->mListenPort;
        } else {
            (*listener)->mPort   = client->mPort;
        }
        (*listener)->mFileName   = NULL;
        (*listener)->mHost       = NULL;
        (*listener)->mLocalhost  = NULL;
        (*listener)->mOutputFileName = NULL;
        (*listener)->mMode       = kTest_Normal;
        (*listener)->mThreadMode = kMode_Listener;
        if ( client->mHost != NULL ) {
            (*listener)->mHost = new char[strlen( client->mHost ) + 1];
            strcpy( (*listener)->mHost, client->mHost );
        }
        if ( client->mLocalhost != NULL ) {
            (*listener)->mLocalhost = new char[strlen( client->mLocalhost ) + 1];
            strcpy( (*listener)->mLocalhost, client->mLocalhost );
        }
    } else {
        *listener = NULL;
    }
}

/*
 * Settings_GenerateSpeakerSettings
 * Called to generate the settings to be passed to the Speaker
 * instance that will handle dual testings from the server side
 * this should only return an instance if it was called on 
 * the thread_Settings instance generated from the command line 
 * for server side execution. This should be an inverse operation
 * of GenerateClientHdr. 
 */
void Settings_GenerateClientSettings( thread_Settings *server, 
                                      thread_Settings **client,
                                      client_hdr *hdr ) {
    int flags = ntohl(hdr->flags);
    if ( (flags & HEADER_VERSION1) != 0 ) {
        *client = new thread_Settings;
        memcpy(*client, server, sizeof( thread_Settings ));
        setCompat( (*client) );
        (*client)->mTID = thread_zeroid();
        (*client)->mPort       = (unsigned short) ntohl(hdr->mPort);
        (*client)->mThreads    = ntohl(hdr->numThreads);
        if ( hdr->bufferlen != 0 ) {
            (*client)->mBufLen = ntohl(hdr->bufferlen);
        }
        if ( hdr->mWinBand != 0 ) {
            if ( isUDP( server ) ) {
                (*client)->mUDPRate = ntohl(hdr->mWinBand);
            } else {
                (*client)->mTCPWin = ntohl(hdr->mWinBand);
            }
        }
        (*client)->mAmount     = ntohl(hdr->mAmount);
        if ( ((*client)->mAmount & 0x80000000) > 0 ) {
            setModeTime( (*client) );
#ifndef WIN32
            (*client)->mAmount |= 0xFFFFFFFF00000000LL;
#else
            (*client)->mAmount |= 0xFFFFFFFF00000000;
#endif
            (*client)->mAmount = -(*client)->mAmount;
        }
        (*client)->mFileName   = NULL;
        (*client)->mHost       = NULL;
        (*client)->mLocalhost  = NULL;
        (*client)->mOutputFileName = NULL;
        (*client)->mMode       = ((flags & RUN_NOW) == 0 ?
                                   kTest_TradeOff : kTest_DualTest);
        (*client)->mThreadMode = kMode_Client;
        if ( server->mLocalhost != NULL ) {
            (*client)->mLocalhost = new char[strlen( server->mLocalhost ) + 1];
            strcpy( (*client)->mLocalhost, server->mLocalhost );
        }
        (*client)->mHost = new char[REPORT_ADDRLEN];
        if ( ((sockaddr*)&server->peer)->sa_family == AF_INET ) {
            inet_ntop( AF_INET, &((sockaddr_in*)&server->peer)->sin_addr, 
                       (*client)->mHost, REPORT_ADDRLEN);
        }
#ifdef HAVE_IPV6
          else {
            inet_ntop( AF_INET6, &((sockaddr_in6*)&server->peer)->sin6_addr, 
                       (*client)->mHost, REPORT_ADDRLEN);
        }
#endif
    } else {
        *client = NULL;
    }
}

/*
 * Settings_GenerateClientHdr
 * Called to generate the client header to be passed to the
 * server that will handle dual testings from the server side
 * This should be an inverse operation of GenerateSpeakerSettings
 */
void Settings_GenerateClientHdr( thread_Settings *client, client_hdr *hdr ) {
    if ( client->mMode != kTest_Normal ) {
        hdr->flags  = htonl(HEADER_VERSION1);
    } else {
        hdr->flags  = 0;
    }
    if ( isBuflenSet( client ) ) {
        hdr->bufferlen = htonl(client->mBufLen);
    } else {
        hdr->bufferlen = 0;
    }
    if ( isUDP( client ) ) {
        hdr->mWinBand  = htonl(client->mUDPRate);
    } else {
        hdr->mWinBand  = htonl(client->mTCPWin);
    }
    if ( client->mListenPort != 0 ) {
        hdr->mPort  = htonl(client->mListenPort);
    } else {
        hdr->mPort  = htonl(client->mPort);
    }
    hdr->numThreads = htonl(client->mThreads);
    if ( isModeTime( client ) ) {
        hdr->mAmount    = htonl(-(long)client->mAmount);
    } else {
        hdr->mAmount    = htonl((long)client->mAmount);
        hdr->mAmount &= htonl( 0x7FFFFFFF );
    }
    if ( client->mMode == kTest_DualTest ) {
        hdr->flags |= htonl(RUN_NOW);
    }
}
