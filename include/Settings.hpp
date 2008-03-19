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
 * Settings.hpp
 * by Mark Gates <mgates@nlanr.net>
 * &  Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * -------------------------------------------------------------------
 * Stores and parses the initial values for all the global variables.
 * -------------------------------------------------------------------
 * headers
 * uses
 *   <stdlib.h>
 *   <assert.h>
 * ------------------------------------------------------------------- */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "headers.h"
#include "Thread.h"

/* -------------------------------------------------------------------
 * constants
 * ------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

// server/client mode
typedef enum ThreadMode {
    kMode_Unknown = 0,
    kMode_Server,
    kMode_Client,
    kMode_Reporter,
    kMode_Listener
} ThreadMode;

// report mode
typedef enum ReportMode {
    kReport_Default = 0,
    kReport_CSV,
    //kReport_XML,
    kReport_MAXIMUM
} ReportMode;

// test mode
typedef enum TestMode {
    kTest_Normal = 0,
    kTest_DualTest,
    kTest_TradeOff,
    kTest_Unknown
} TestMode;

#include "Reporter.h"
/*
 * The thread_Settings is a structure that holds all
 * options for a given execution of either a client
 * or server. By using this structure rather than
 * a global structure or class we can have multiple
 * clients or servers running with different settings.
 * In version 2.0 and above this structure contains 
 * all the information needed for a thread to execute
 * and contains only C elements so it can be manipulated
 * by either C or C++.
 */
typedef struct thread_Settings {
    // Pointers
    char*  mFileName;               // -F
    char*  mHost;                   // -c
    char*  mLocalhost;              // -B
    char*  mOutputFileName;         // -o
    FILE*  Extractor_file;
    ReportHeader*  reporthdr;
    MultiHeader*   multihdr;
    struct thread_Settings *runNow;
    struct thread_Settings *runNext;
    // int's
    int mThreads;                   // -P
    int mTOS;                       // -S
    int mSock;
    int Extractor_size;
    int mBufLen;                    // -l
    int mMSS;                       // -M
    int mTCPWin;                    // -w
    /*   flags is a BitMask of old bools
        bool   mBufLenSet;              // -l
        bool   mCompat;                 // -C
        bool   mDaemon;                 // -D
        bool   mDomain;                 // -V
        bool   mFileInput;              // -F or -I
        bool   mNodelay;                // -N
        bool   mPrintMSS;               // -m
        bool   mRemoveService;          // -R
        bool   mStdin;                  // -I
        bool   mStdout;                 // -o
        bool   mSuggestWin;             // -W
        bool   mUDP;                    // -u
        bool   mMode_time;
        bool   mReportSettings;
        bool   mMulticast;
        bool   mNoSettingsReport;       // -x s
        bool   mNoConnectionReport;     // -x c
        bool   mNoDataReport;           // -x d
        bool   mNoServerReport;         // -x 
        bool   mNoMultReport;           // -x m
        bool   mSinlgeClient;           // -1 */
    int flags; 
    // enums (which should be special int's)
    ThreadMode mThreadMode;         // -s or -c
    ReportMode mReportMode;
    TestMode mMode;                 // -r or -d
    // Hopefully int64_t's
    max_size_t mUDPRate;            // -b or -u
    max_size_t mAmount;             // -n or -t
    // doubles
    double mInterval;               // -i
    // shorts
    unsigned short mListenPort;     // -L
    unsigned short mPort;           // -p
    // chars
    char   mFormat;                 // -f
    int mTTL;                    // -T
    char pad1[2];
    // structs or miscellaneous
    iperf_sockaddr peer;
    Socklen_t size_peer;
    iperf_sockaddr local;
    Socklen_t size_local;
    nthread_t mTID;
    char* mCongestion;
#if defined( HAVE_WIN32_THREAD )
    HANDLE mHandle;
#endif
} thread_Settings;

/*
 * Due to the use of thread_Settings in C and C++
 * we are unable to use bool values. To provide
 * the functionality of bools we use the following
 * bitmask over an assumed 32 bit int. This will
 * work fine on 64bit machines we will just be ignoring
 * the upper 32bits.
 *
 * To add a flag simply define it as the next bit then
 * add the 3 support functions below.
 */
#define FLAG_BUFLENSET      0x00000001
#define FLAG_COMPAT         0x00000002
#define FLAG_DAEMON         0x00000004
#define FLAG_DOMAIN         0x00000008
#define FLAG_FILEINPUT      0x00000010
#define FLAG_NODELAY        0x00000020
#define FLAG_PRINTMSS       0x00000040
#define FLAG_REMOVESERVICE  0x00000080
#define FLAG_STDIN          0x00000100
#define FLAG_STDOUT         0x00000200
#define FLAG_SUGGESTWIN     0x00000400
#define FLAG_UDP            0x00000800
#define FLAG_MODETIME       0x00001000
#define FLAG_REPORTSETTINGS 0x00002000
#define FLAG_MULTICAST      0x00004000
#define FLAG_NOSETTREPORT   0x00008000
#define FLAG_NOCONNREPORT   0x00010000
#define FLAG_NODATAREPORT   0x00020000
#define FLAG_NOSERVREPORT   0x00040000
#define FLAG_NOMULTREPORT   0x00080000
#define FLAG_SINGLECLIENT   0x00100000
#define FLAG_SINGLEUDP      0x00200000
#define FLAG_CONGESTION     0x00400000

#define isBuflenSet(settings)      ((settings->flags & FLAG_BUFLENSET) != 0)
#define isCompat(settings)         ((settings->flags & FLAG_COMPAT) != 0)
#define isDaemon(settings)         ((settings->flags & FLAG_DAEMON) != 0)
#define isIPV6(settings)           ((settings->flags & FLAG_DOMAIN) != 0)
#define isFileInput(settings)      ((settings->flags & FLAG_FILEINPUT) != 0)
#define isNoDelay(settings)        ((settings->flags & FLAG_NODELAY) != 0)
#define isPrintMSS(settings)       ((settings->flags & FLAG_PRINTMSS) != 0)
#define isRemoveService(settings)  ((settings->flags & FLAG_REMOVESERVICE) != 0)
#define isSTDIN(settings)          ((settings->flags & FLAG_STDIN) != 0)
#define isSTDOUT(settings)         ((settings->flags & FLAG_STDOUT) != 0)
#define isSuggestWin(settings)     ((settings->flags & FLAG_SUGGESTWIN) != 0)
#define isUDP(settings)            ((settings->flags & FLAG_UDP) != 0)
#define isModeTime(settings)       ((settings->flags & FLAG_MODETIME) != 0)
#define isReport(settings)         ((settings->flags & FLAG_REPORTSETTINGS) != 0)
#define isMulticast(settings)      ((settings->flags & FLAG_MULTICAST) != 0)
// Active Low for Reports
#define isSettingsReport(settings) ((settings->flags & FLAG_NOSETTREPORT) == 0)
#define isConnectionReport(settings)  ((settings->flags & FLAG_NOCONNREPORT) == 0)
#define isDataReport(settings)     ((settings->flags & FLAG_NODATAREPORT) == 0)
#define isServerReport(settings)   ((settings->flags & FLAG_NOSERVREPORT) == 0)
#define isMultipleReport(settings) ((settings->flags & FLAG_NOMULTREPORT) == 0)
// end Active Low
#define isSingleClient(settings)   ((settings->flags & FLAG_SINGLECLIENT) != 0)
#define isSingleUDP(settings)      ((settings->flags & FLAG_SINGLEUDP) != 0)
#define isCongestionControl(settings) ((settings->flags & FLAG_CONGESTION) != 0)

#define setBuflenSet(settings)     settings->flags |= FLAG_BUFLENSET
#define setCompat(settings)        settings->flags |= FLAG_COMPAT
#define setDaemon(settings)        settings->flags |= FLAG_DAEMON
#define setIPV6(settings)          settings->flags |= FLAG_DOMAIN
#define setFileInput(settings)     settings->flags |= FLAG_FILEINPUT
#define setNoDelay(settings)       settings->flags |= FLAG_NODELAY
#define setPrintMSS(settings)      settings->flags |= FLAG_PRINTMSS
#define setRemoveService(settings) settings->flags |= FLAG_REMOVESERVICE
#define setSTDIN(settings)         settings->flags |= FLAG_STDIN
#define setSTDOUT(settings)        settings->flags |= FLAG_STDOUT
#define setSuggestWin(settings)    settings->flags |= FLAG_SUGGESTWIN
#define setUDP(settings)           settings->flags |= FLAG_UDP
#define setModeTime(settings)      settings->flags |= FLAG_MODETIME
#define setReport(settings)        settings->flags |= FLAG_REPORTSETTINGS
#define setMulticast(settings)     settings->flags |= FLAG_MULTICAST
#define setNoSettReport(settings)  settings->flags |= FLAG_NOSETTREPORT
#define setNoConnReport(settings)  settings->flags |= FLAG_NOCONNREPORT
#define setNoDataReport(settings)  settings->flags |= FLAG_NODATAREPORT
#define setNoServReport(settings)  settings->flags |= FLAG_NOSERVREPORT
#define setNoMultReport(settings)  settings->flags |= FLAG_NOMULTREPORT
#define setSingleClient(settings)  settings->flags |= FLAG_SINGLECLIENT
#define setSingleUDP(settings)     settings->flags |= FLAG_SINGLEUDP
#define setCongestionControl(settings) settings->flags |= FLAG_CONGESTION

#define unsetBuflenSet(settings)   settings->flags &= ~FLAG_BUFLENSET
#define unsetCompat(settings)      settings->flags &= ~FLAG_COMPAT
#define unsetDaemon(settings)      settings->flags &= ~FLAG_DAEMON
#define unsetIPV6(settings)        settings->flags &= ~FLAG_DOMAIN
#define unsetFileInput(settings)   settings->flags &= ~FLAG_FILEINPUT
#define unsetNoDelay(settings)     settings->flags &= ~FLAG_NODELAY
#define unsetPrintMSS(settings)    settings->flags &= ~FLAG_PRINTMSS
#define unsetRemoveService(settings)  settings->flags &= ~FLAG_REMOVESERVICE
#define unsetSTDIN(settings)       settings->flags &= ~FLAG_STDIN
#define unsetSTDOUT(settings)      settings->flags &= ~FLAG_STDOUT
#define unsetSuggestWin(settings)  settings->flags &= ~FLAG_SUGGESTWIN
#define unsetUDP(settings)         settings->flags &= ~FLAG_UDP
#define unsetModeTime(settings)    settings->flags &= ~FLAG_MODETIME
#define unsetReport(settings)      settings->flags &= ~FLAG_REPORTSETTINGS
#define unsetMulticast(settings)   settings->flags &= ~FLAG_MULTICAST
#define unsetNoSettReport(settings)   settings->flags &= ~FLAG_NOSETTREPORT
#define unsetNoConnReport(settings)   settings->flags &= ~FLAG_NOCONNREPORT
#define unsetNoDataReport(settings)   settings->flags &= ~FLAG_NODATAREPORT
#define unsetNoServReport(settings)   settings->flags &= ~FLAG_NOSERVREPORT
#define unsetNoMultReport(settings)   settings->flags &= ~FLAG_NOMULTREPORT
#define unsetSingleClient(settings)   settings->flags &= ~FLAG_SINGLECLIENT
#define unsetSingleUDP(settings)      settings->flags &= ~FLAG_SINGLEUDP
#define unsetCongestionControl(settings) settings->flags &= ~FLAG_CONGESTION


#define HEADER_VERSION1 0x80000000
#define RUN_NOW         0x00000001

// used to reference the 4 byte ID number we place in UDP datagrams
// use int32_t if possible, otherwise a 32 bit bitfield (e.g. on J90) 
typedef struct UDP_datagram {
#ifdef HAVE_INT32_T
    int32_t id;
    u_int32_t tv_sec;
    u_int32_t tv_usec;
#else
    signed   int id      : 32;
    unsigned int tv_sec  : 32;
    unsigned int tv_usec : 32;
#endif
} UDP_datagram;

/*
 * The client_hdr structure is sent from clients
 * to servers to alert them of things that need
 * to happen. Order must be perserved in all 
 * future releases for backward compatibility.
 * 1.7 has flags, numThreads, mPort, and bufferlen
 */
typedef struct client_hdr {

#ifdef HAVE_INT32_T

    /*
     * flags is a bitmap for different options
     * the most significant bits are for determining
     * which information is available. So 1.7 uses
     * 0x80000000 and the next time information is added
     * the 1.7 bit will be set and 0x40000000 will be
     * set signifying additional information. If no 
     * information bits are set then the header is ignored.
     * The lowest order diferentiates between dualtest and
     * tradeoff modes, wheither the speaker needs to start 
     * immediately or after the audience finishes.
     */
    int32_t flags;
    int32_t numThreads;
    int32_t mPort;
    int32_t bufferlen;
    int32_t mWinBand;
    int32_t mAmount;
#else
    signed int flags      : 32;
    signed int numThreads : 32;
    signed int mPort      : 32;
    signed int bufferlen  : 32;
    signed int mWinBand   : 32;
    signed int mAmount    : 32;
#endif
} client_hdr;

/*
 * The server_hdr structure facilitates the server
 * report of jitter and loss on the client side.
 * It piggy_backs on the existing clear to close
 * packet.
 */
typedef struct server_hdr {

#ifdef HAVE_INT32_T

    /*
     * flags is a bitmap for different options
     * the most significant bits are for determining
     * which information is available. So 1.7 uses
     * 0x80000000 and the next time information is added
     * the 1.7 bit will be set and 0x40000000 will be
     * set signifying additional information. If no 
     * information bits are set then the header is ignored.
     */
    int32_t flags;
    int32_t total_len1;
    int32_t total_len2;
    int32_t stop_sec;
    int32_t stop_usec;
    int32_t error_cnt;
    int32_t outorder_cnt;
    int32_t datagrams;
    int32_t jitter1;
    int32_t jitter2;
#else
    signed int flags        : 32;
    signed int total_len1   : 32;
    signed int total_len2   : 32;
    signed int stop_sec     : 32;
    signed int stop_usec    : 32;
    signed int error_cnt    : 32;
    signed int outorder_cnt : 32;
    signed int datagrams    : 32;
    signed int jitter1      : 32;
    signed int jitter2      : 32;
#endif

} server_hdr;

    // set to defaults
    void Settings_Initialize( thread_Settings* main );

    // copy structure
    void Settings_Copy( thread_Settings* from, thread_Settings** into );

    // free associated memory
    void Settings_Destroy( thread_Settings *mSettings );

    // parse settings from user's environment variables
    void Settings_ParseEnvironment( thread_Settings *mSettings );

    // parse settings from app's command line
    void Settings_ParseCommandLine( int argc, char **argv, thread_Settings *mSettings );

    // convert to lower case for [KMG]bits/sec
    void Settings_GetLowerCaseArg(const char *,char *);

    // convert to upper case for [KMG]bytes/sec
    void Settings_GetUpperCaseArg(const char *,char *);

    // generate settings for listener instance
    void Settings_GenerateListenerSettings( thread_Settings *client, thread_Settings **listener);

    // generate settings for speaker instance
    void Settings_GenerateClientSettings( thread_Settings *server, 
                                          thread_Settings **client,
                                          client_hdr *hdr );

    // generate client header for server
    void Settings_GenerateClientHdr( thread_Settings *client, client_hdr *hdr );

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // SETTINGS_H
