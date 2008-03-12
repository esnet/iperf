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
 * Reporter.h
 * by Kevin Gibbs <kgibbs@nlanr.net>
 *
 * Since version 2.0 this handles all reporting.
 * ________________________________________________________________ */

#ifndef REPORTER_H
#define REPORTER_H

#include "headers.h"
#include "Mutex.h"

struct thread_Settings;
struct server_hdr;

#include "Settings.hpp"

#define NUM_REPORT_STRUCTS 700
#define NUM_MULTI_SLOTS    5

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This struct contains all important information from the sending or
 * recieving thread.
 */
typedef struct ReportStruct {
    int packetID;
    max_size_t packetLen;
    struct timeval packetTime;
    struct timeval sentTime;
} ReportStruct;

/*
 * The type field of ReporterData is a bitmask
 * with one or more of the following
 */
#define    TRANSFER_REPORT      0x00000001
#define    SERVER_RELAY_REPORT  0x00000002
#define    SETTINGS_REPORT      0x00000004
#define    CONNECTION_REPORT    0x00000008
#define    MULTIPLE_REPORT      0x00000010

typedef struct Transfer_Info {
    void *reserved_delay;
    int transferID;
    int groupID;
    int cntError;
    int cntOutofOrder;
    int cntDatagrams;
    // Hopefully int64_t's
    max_size_t TotalLen;
    double jitter;
    double startTime;
    double endTime;
    // chars
    char   mFormat;                 // -f
    u_char mTTL;                    // -T
    char   mUDP;
    char   free;
} Transfer_Info;

typedef struct Connection_Info {
    iperf_sockaddr peer;
    Socklen_t size_peer;
    iperf_sockaddr local;
    Socklen_t size_local;
} Connection_Info;

typedef struct ReporterData {
    char*  mHost;                   // -c
    char*  mLocalhost;              // -B
    // int's
    int type;
    int cntError;
    int lastError;
    int cntOutofOrder;
    int lastOutofOrder;
    int cntDatagrams;
    int lastDatagrams;
    int PacketID;
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
        bool   mUDP;
        bool   mMode_time;*/
    int flags; 
    // enums (which should be special int's)
    ThreadMode mThreadMode;         // -s or -c
    ReportMode mode;
    max_size_t TotalLen;
    max_size_t lastTotal;
    // doubles
    double lastTransit;
    // shorts
    unsigned short mPort;           // -p
    // structs or miscellaneous
    Transfer_Info info;
    Connection_Info connection;
    struct timeval startTime;
    struct timeval packetTime;
    struct timeval nextTime;
    struct timeval intervalTime;
} ReporterData;

typedef struct MultiHeader {
    int reporterindex;
    int agentindex;
    int groupID;
    int threads;
    ReporterData *report;
    Transfer_Info *data;
    Condition barrier;
    struct timeval startTime;
} MultiHeader;

typedef struct ReportHeader {
    int reporterindex;
    int agentindex;
    ReporterData report;
    ReportStruct *data;
    MultiHeader *multireport;
    struct ReportHeader *next;
} ReportHeader;

typedef void* (* report_connection)( Connection_Info*, int );
typedef void (* report_settings)( ReporterData* );
typedef void (* report_statistics)( Transfer_Info* );
typedef void (* report_serverstatistics)( Connection_Info*, Transfer_Info* );

MultiHeader* InitMulti( struct thread_Settings *agent, int inID );
ReportHeader* InitReport( struct thread_Settings *agent );
void ReportPacket( ReportHeader *agent, ReportStruct *packet );
void CloseReport( ReportHeader *agent, ReportStruct *packet );
void EndReport( ReportHeader *agent );
Transfer_Info* GetReport( ReportHeader *agent );
void ReportServerUDP( struct thread_Settings *agent, struct server_hdr *server );
void ReportSettings( struct thread_Settings *agent );
void ReportConnections( struct thread_Settings *agent );

extern report_connection connection_reports[];

extern report_settings settings_reports[];

extern report_statistics statistics_reports[];

extern report_serverstatistics serverstatistics_reports[];

extern report_statistics multiple_reports[];

extern char buffer[64]; // Buffer for printing

#define rMillion 1000000

#define TimeDifference( left, right ) (left.tv_sec  - right.tv_sec) +   \
        (left.tv_usec - right.tv_usec) / ((double) rMillion)

#define TimeAdd( left, right )  do {                                    \
                                    left.tv_usec += right.tv_usec;      \
                                    if ( left.tv_usec > rMillion ) {    \
                                        left.tv_usec -= rMillion;       \
                                        left.tv_sec++;                  \
                                    }                                   \
                                    left.tv_sec += right.tv_sec;        \
                                } while ( 0 )
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // REPORTER_H
