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
 * Reporter.c
 * by Kevin Gibbs <kgibbs@nlanr.net>
 *
 * ________________________________________________________________ */

#include "headers.h"
#include "Settings.hpp"
#include "util.h"
#include "Reporter.h"
#include "Thread.h"
#include "Locale.h"
#include "PerfSocket.hpp"
#include "SocketAddr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  The following 4 functions are provided for Reporting
  styles that do not have all the reporting formats. For
  instance the provided CSV format does not have a settings
  report so it uses settings_notimpl.
  */
void* connection_notimpl( Connection_Info * nused, int nuse ) { 
    return NULL; 
}
void settings_notimpl( ReporterData * nused ) { }
void statistics_notimpl( Transfer_Info * nused ) { }
void serverstatistics_notimpl( Connection_Info *nused1, Transfer_Info *nused2 ) { }

// To add a reporting style include its header here.
#include "report_default.h"
#include "report_CSV.h"

// The following array of report structs contains the
// pointers required for reporting in different reporting
// styles. To add a reporting style add a report struct
// below.
report_connection connection_reports[kReport_MAXIMUM] = {
    reporter_reportpeer,
    CSV_peer
};

report_settings settings_reports[kReport_MAXIMUM] = {
    reporter_reportsettings,
    settings_notimpl
};

report_statistics statistics_reports[kReport_MAXIMUM] = {
    reporter_printstats,
    CSV_stats
};

report_serverstatistics serverstatistics_reports[kReport_MAXIMUM] = {
    reporter_serverstats,
    CSV_serverstats
};

report_statistics multiple_reports[kReport_MAXIMUM] = {
    reporter_multistats,
    CSV_stats
};

char buffer[64]; // Buffer for printing
ReportHeader *ReportRoot = NULL;
int threadWait = 0;
int threadSleeping = 0;
extern Condition ReportCond;
int reporter_process_report ( ReportHeader *report );
void process_report ( ReportHeader *report );
int reporter_handle_packet( ReportHeader *report );
int reporter_condprintstats( ReporterData *stats, MultiHeader *multireport, int force );
int reporter_print( ReporterData *stats, int type, int end );
void PrintMSS( ReporterData *stats );

MultiHeader* InitMulti( thread_Settings *agent, int inID ) {
    MultiHeader *multihdr = NULL;
    if ( agent->mThreads > 1 || agent->mThreadMode == kMode_Server ) {
        if ( isMultipleReport( agent ) ) {
            multihdr = malloc(sizeof(MultiHeader) +  sizeof(ReporterData) +
                              NUM_MULTI_SLOTS * sizeof(Transfer_Info));
        } else {
            multihdr = malloc(sizeof(MultiHeader));
        }
        if ( multihdr != NULL ) {
            memset( multihdr, 0, sizeof(MultiHeader) );
            Condition_Initialize( &multihdr->barrier );
            multihdr->groupID = inID;
            multihdr->threads = agent->mThreads;
            if ( isMultipleReport( agent ) ) {
                int i;
                ReporterData *data = NULL;
                multihdr->report = (ReporterData*)(multihdr + 1);
                memset(multihdr->report, 0, sizeof(ReporterData));
                multihdr->data = (Transfer_Info*)(multihdr->report + 1);
                data = multihdr->report;
                for ( i = 0; i < NUM_MULTI_SLOTS; i++ ) {
                    multihdr->data[i].startTime = -1;
                    multihdr->data[i].transferID = inID;
                    multihdr->data[i].groupID = -2;
                }
                data->type = TRANSFER_REPORT;
                if ( agent->mInterval != 0.0 ) {
                    struct timeval *interval = &data->intervalTime;
                    interval->tv_sec = (long) agent->mInterval;
                    interval->tv_usec = (long) ((agent->mInterval - interval->tv_sec) 
                                                * rMillion);
                }
                data->mHost = agent->mHost;
                data->mLocalhost = agent->mLocalhost;
                data->mBufLen = agent->mBufLen;
                data->mMSS = agent->mMSS;
                data->mTCPWin = agent->mTCPWin;
                data->flags = agent->flags;
                data->mThreadMode = agent->mThreadMode;
                data->mode = agent->mReportMode;
                data->info.mFormat = agent->mFormat;
                data->info.mTTL = agent->mTTL;
                if ( isUDP( agent ) ) {
                    multihdr->report->info.mUDP = (char)agent->mThreadMode;
                }
                if ( isConnectionReport( agent ) ) {
                    data->type |= CONNECTION_REPORT;
                    data->connection.peer = agent->peer;
                    data->connection.size_peer = agent->size_peer;
                    SockAddr_setPortAny( &data->connection.peer );
                    data->connection.local = agent->local;
                    data->connection.size_local = agent->size_local;
                    SockAddr_setPortAny( &data->connection.local );
                }
            }
        } else {
            FAIL(1, "Out of Memory!!\n", agent);
        }
    }
    return multihdr;
}

/*
 * BarrierClient allows for multiple stream clients to be syncronized
 */
void BarrierClient( ReportHeader *agent ) {
    Condition_Lock(agent->multireport->barrier);
    agent->multireport->threads--;
    if ( agent->multireport->threads == 0 ) {
        // last one set time and wake up everyone
        gettimeofday( &(agent->multireport->startTime), NULL );
        Condition_Broadcast( &agent->multireport->barrier );
    } else {
        Condition_Wait( &agent->multireport->barrier );
    }
    agent->multireport->threads++;
    Condition_Unlock( agent->multireport->barrier );
    agent->report.startTime = agent->multireport->startTime;
    agent->report.nextTime = agent->report.startTime;
    TimeAdd( agent->report.nextTime, agent->report.intervalTime );
}

/*
 * InitReport is called by a transfer agent (client or
 * server) to setup the needed structures to communicate
 * traffic.
 */
ReportHeader* InitReport( thread_Settings *agent ) {
    ReportHeader *reporthdr = NULL;
    ReporterData *data = NULL;
    if ( isDataReport( agent ) ) {
        /*
         * Create in one big chunk
         */
        reporthdr = malloc( sizeof(ReportHeader) +
                            NUM_REPORT_STRUCTS * sizeof(ReportStruct) );
        if ( reporthdr != NULL ) {
            // Only need to make sure the headers are clean
            memset( reporthdr, 0, sizeof(ReportHeader));
            reporthdr->data = (ReportStruct*)(reporthdr+1);
            reporthdr->multireport = agent->multihdr;
            data = &reporthdr->report;
            reporthdr->reporterindex = NUM_REPORT_STRUCTS - 1;
            data->info.transferID = agent->mSock;
            data->info.groupID = (agent->multihdr != NULL ? agent->multihdr->groupID 
                                                          : -1);
            data->type = TRANSFER_REPORT;
            if ( agent->mInterval != 0.0 ) {
                struct timeval *interval = &data->intervalTime;
                interval->tv_sec = (long) agent->mInterval;
                interval->tv_usec = (long) ((agent->mInterval - interval->tv_sec) 
                                            * rMillion);
            }
            data->mHost = agent->mHost;
            data->mLocalhost = agent->mLocalhost;
            data->mBufLen = agent->mBufLen;
            data->mMSS = agent->mMSS;
            data->mTCPWin = agent->mTCPWin;
            data->flags = agent->flags;
            data->mThreadMode = agent->mThreadMode;
            data->mode = agent->mReportMode;
            data->info.mFormat = agent->mFormat;
            data->info.mTTL = agent->mTTL;
            if ( isUDP( agent ) ) {
                reporthdr->report.info.mUDP = (char)agent->mThreadMode;
            }
        } else {
            FAIL(1, "Out of Memory!!\n", agent);
        }
    }
    if ( isConnectionReport( agent ) ) {
        if ( reporthdr == NULL ) {
            /*
             * Create in one big chunk
             */
            reporthdr = malloc( sizeof(ReportHeader) );
            if ( reporthdr != NULL ) {
                // Only need to make sure the headers are clean
                memset( reporthdr, 0, sizeof(ReportHeader));
                data = &reporthdr->report;
                data->info.transferID = agent->mSock;
                data->info.groupID = -1;
            } else {
                FAIL(1, "Out of Memory!!\n", agent);
            }
        }
        if ( reporthdr != NULL ) {
            data->type |= CONNECTION_REPORT;
            data->connection.peer = agent->peer;
            data->connection.size_peer = agent->size_peer;
            data->connection.local = agent->local;
            data->connection.size_local = agent->size_local;
        } else {
            FAIL(1, "Out of Memory!!\n", agent);
        }
    }
    if ( isConnectionReport( agent ) || isDataReport( agent ) ) {

#ifdef HAVE_THREAD
        /*
         * Update the ReportRoot to include this report.
         */
        if ( reporthdr->report.mThreadMode == kMode_Client &&
             reporthdr->multireport != NULL ) {
            // syncronize watches on my mark......
            BarrierClient( reporthdr );
        } else {
            if ( reporthdr->multireport != NULL && isMultipleReport( agent )) {
                reporthdr->multireport->threads++;
                if ( reporthdr->multireport->report->startTime.tv_sec == 0 ) {
                    gettimeofday( &(reporthdr->multireport->report->startTime), NULL );
                }
                reporthdr->report.startTime = reporthdr->multireport->report->startTime;
            } else {
                // set start time
                gettimeofday( &(reporthdr->report.startTime), NULL );
            }
            reporthdr->report.nextTime = reporthdr->report.startTime;
            TimeAdd( reporthdr->report.nextTime, reporthdr->report.intervalTime );
        }
        Condition_Lock( ReportCond );
        reporthdr->next = ReportRoot;
        ReportRoot = reporthdr;
        Condition_Signal( &ReportCond );
        Condition_Unlock( ReportCond );
#else
        // set start time
        gettimeofday( &(reporthdr->report.startTime), NULL );
        /*
         * Process the report in this thread
         */
        reporthdr->next = NULL;
        process_report ( reporthdr );
#endif 
    }
    if ( !isDataReport( agent ) ) {
        reporthdr = NULL;
    }
    return reporthdr;
}

/*
 * ReportPacket is called by a transfer agent to record
 * the arrival or departure of a "packet" (for TCP it 
 * will actually represent many packets). This needs to
 * be as simple and fast as possible as it gets called for
 * every "packet".
 */
void ReportPacket( ReportHeader* agent, ReportStruct *packet ) {
    if ( agent != NULL ) {
        int index = agent->reporterindex;
        /*
         * First find the appropriate place to put the information
         */
        if ( agent->agentindex == NUM_REPORT_STRUCTS ) {
            // Just need to make sure that reporter is not on the first
            // item
            while ( index == 0 ) {
                Condition_Signal( &ReportCond );
                thread_rest();
                index = agent->reporterindex;
            }
            agent->agentindex = 0;
        }
        // Need to make sure that reporter is not about to be "lapped"
        while ( index - 1 == agent->agentindex ) {
            Condition_Signal( &ReportCond );
            thread_rest();
            index = agent->reporterindex;
        }
	if (threadSleeping)
           Condition_Signal( &ReportCond );

        // Put the information there
        memcpy( agent->data + agent->agentindex, packet, sizeof(ReportStruct) );
        
        // Updating agentindex MUST be the last thing done
        agent->agentindex++;
#ifndef HAVE_THREAD
        /*
         * Process the report in this thread
         */
        process_report ( agent );
#endif 
    }
}

/*
 * CloseReport is called by a transfer agent to finalize
 * the report and signal transfer is over.
 */
void CloseReport( ReportHeader *agent, ReportStruct *packet ) {
    if ( agent != NULL) {

        /*
         * Using PacketID of -1 ends reporting
         */
        packet->packetID = -1;
        packet->packetLen = 0;
        ReportPacket( agent, packet );
        packet->packetID = agent->report.cntDatagrams;
	if (threadSleeping)
           Condition_Signal( &ReportCond );

    }
}

/*
 * EndReport signifies the agent no longer is interested
 * in the report. Calls to GetReport will no longer be
 * filled
 */
void EndReport( ReportHeader *agent ) {
    if ( agent != NULL ) {
        int index = agent->reporterindex;
	if (threadSleeping)
           Condition_Signal( &ReportCond );

        while ( index != -1 ) {
            thread_rest();
            index = agent->reporterindex;
        }
        agent->agentindex = -1;
#ifndef HAVE_THREAD
        /*
         * Process the report in this thread
         */
        process_report ( agent );
#endif
    }
}

/*
 * GetReport is called by the agent after a CloseReport
 * but before an EndReport to get the stats generated
 * by the reporter thread.
 */
Transfer_Info *GetReport( ReportHeader *agent ) {
    int index = agent->reporterindex;
    while ( index != -1 ) {
        thread_rest();
        index = agent->reporterindex;
    }
    return &agent->report.info;
}

/*
 * ReportSettings will generate a summary report for
 * settings being used with Listeners or Clients
 */
void ReportSettings( thread_Settings *agent ) {
    if ( isSettingsReport( agent ) ) {
        /*
         * Create in one big chunk
         */
        ReportHeader *reporthdr = malloc( sizeof(ReportHeader) );
    
        if ( reporthdr != NULL ) {
            ReporterData *data = &reporthdr->report;
            data->info.transferID = agent->mSock;
            data->info.groupID = -1;
            reporthdr->agentindex = -1;
            reporthdr->reporterindex = -1;
        
            data->mHost = agent->mHost;
            data->mLocalhost = agent->mLocalhost;
            data->mode = agent->mReportMode;
            data->type = SETTINGS_REPORT;
            data->mBufLen = agent->mBufLen;
            data->mMSS = agent->mMSS;
            data->mTCPWin = agent->mTCPWin;
            data->flags = agent->flags;
            data->mThreadMode = agent->mThreadMode;
            data->mPort = agent->mPort;
            data->info.mFormat = agent->mFormat;
            data->info.mTTL = agent->mTTL;
            data->connection.peer = agent->peer;
            data->connection.size_peer = agent->size_peer;
            data->connection.local = agent->local;
            data->connection.size_local = agent->size_local;
    
    #ifdef HAVE_THREAD
            /*
             * Update the ReportRoot to include this report.
             */
            Condition_Lock( ReportCond );
	    if ( isUDP(agent) )
	      threadWait = 0;
	    else
	      threadWait = 1;
            reporthdr->next = ReportRoot;
            ReportRoot = reporthdr;
            Condition_Signal( &ReportCond );
            Condition_Unlock( ReportCond );
    #else
            /*
             * Process the report in this thread
             */
            reporthdr->next = NULL;
            process_report ( reporthdr );
    #endif 
        } else {
            FAIL(1, "Out of Memory!!\n", agent);
        }
    }
}

/*
 * ReportServerUDP will generate a report of the UDP
 * statistics as reported by the server on the client
 * side.
 */
void ReportServerUDP( thread_Settings *agent, server_hdr *server ) {
    if ( (ntohl(server->flags) & HEADER_VERSION1) != 0 &&
         isServerReport( agent ) ) {
        /*
         * Create in one big chunk
         */
        ReportHeader *reporthdr = malloc( sizeof(ReportHeader) );
        Transfer_Info *stats = &reporthdr->report.info;

        if ( reporthdr != NULL ) {
            stats->transferID = agent->mSock;
            stats->groupID = (agent->multihdr != NULL ? agent->multihdr->groupID 
                                                      : -1);
            reporthdr->agentindex = -1;
            reporthdr->reporterindex = -1;

            reporthdr->report.type = SERVER_RELAY_REPORT;
            reporthdr->report.mode = agent->mReportMode;
            stats->mFormat = agent->mFormat;
            stats->jitter = ntohl( server->jitter1 );
            stats->jitter += ntohl( server->jitter2 ) / (double)rMillion;
            stats->TotalLen = (((max_size_t) ntohl( server->total_len1 )) << 32) +
                                  ntohl( server->total_len2 ); 
            stats->startTime = 0;
            stats->endTime = ntohl( server->stop_sec );
            stats->endTime += ntohl( server->stop_usec ) / (double)rMillion;
            stats->cntError = ntohl( server->error_cnt );
            stats->cntOutofOrder = ntohl( server->outorder_cnt );
            stats->cntDatagrams = ntohl( server->datagrams );
            stats->mUDP = (char)kMode_Server;
            reporthdr->report.connection.peer = agent->local;
            reporthdr->report.connection.size_peer = agent->size_local;
            reporthdr->report.connection.local = agent->peer;
            reporthdr->report.connection.size_local = agent->size_peer;
            
#ifdef HAVE_THREAD
            /*
             * Update the ReportRoot to include this report.
             */
            Condition_Lock( ReportCond );
            reporthdr->next = ReportRoot;
            ReportRoot = reporthdr;
            Condition_Signal( &ReportCond );
            Condition_Unlock( ReportCond );
#else
            /*
             * Process the report in this thread
             */
            reporthdr->next = NULL;
            process_report ( reporthdr );
#endif 
        } else {
            FAIL(1, "Out of Memory!!\n", agent);
        }
    }
}

/*
 * This function is called only when the reporter thread
 * This function is the loop that the reporter thread processes
 */
void reporter_spawn( thread_Settings *thread ) {
    do {
        // This section allows for safe exiting with Ctrl-C
        Condition_Lock ( ReportCond );
        if ( ReportRoot == NULL ) {
            // Allow main thread to exit if Ctrl-C is received
            thread_setignore();
            Condition_Wait ( &ReportCond );
            // Stop main thread from exiting until done with all reports
            thread_unsetignore();
        }
        Condition_Unlock ( ReportCond );

        if ( ReportRoot != NULL ) {
            ReportHeader *temp = ReportRoot;
            //Condition_Unlock ( ReportCond );
            if ( reporter_process_report ( temp ) ) {
                // This section allows for more reports to be added while
                // the reporter is processing reports without needing to
                // stop the reporter or immediately notify it
                Condition_Lock ( ReportCond );
                if ( temp == ReportRoot ) {
                    // no new reports
                    ReportRoot = temp->next;
                } else {
                    // new reports added
                    ReportHeader *itr = ReportRoot;
                    while ( itr->next != temp ) {
                        itr = itr->next;
                    }
                    itr->next = temp->next;
                }
                // finished with report so free it
                free( temp );
                Condition_Unlock ( ReportCond );
            }
            // yield control of CPU is another thread is waiting
	    // sleep on a condition variable, as it is much cheaper
	    // on most platforms than issuing schedyield or usleep
	    // syscalls
	    Condition_Lock ( ReportCond );
	    if ( threadWait && ReportRoot != NULL) {
	      threadSleeping = 1;
	      Condition_TimedWait (& ReportCond, 1 );
	      threadSleeping = 0;
	    }
	    Condition_Unlock ( ReportCond );
	    
        } else {
            //Condition_Unlock ( ReportCond );
        }
    } while ( 1 );
}

/*
 * Used for single threaded reporting
 */
void process_report ( ReportHeader *report ) {
    if ( report != NULL ) {
        if ( reporter_process_report( report ) ) {
            free( report );
        }
    }
}

/*
 * Process reports starting with "reporthdr"
 */
int reporter_process_report ( ReportHeader *reporthdr ) {
    int need_free = 0;

    // Recursively process reports
    if ( reporthdr->next != NULL ) {
        if ( reporter_process_report( reporthdr->next ) ) {
            // If we are done with this report then free it
            ReportHeader *temp = reporthdr->next;
            reporthdr->next = reporthdr->next->next;
            free( temp );
        }
    }

    if ( (reporthdr->report.type & SETTINGS_REPORT) != 0 ) {
        reporthdr->report.type &= ~SETTINGS_REPORT;
        return reporter_print( &reporthdr->report, SETTINGS_REPORT, 1 );
    } else if ( (reporthdr->report.type & CONNECTION_REPORT) != 0 ) {
        reporthdr->report.type &= ~CONNECTION_REPORT;
        reporter_print( &reporthdr->report, CONNECTION_REPORT,
                               (reporthdr->report.type == 0 ? 1 : 0) );
        if ( reporthdr->multireport != NULL && isMultipleReport( (&reporthdr->report) )) {
            if ( (reporthdr->multireport->report->type & CONNECTION_REPORT) != 0 ) {
                reporthdr->multireport->report->type &= ~CONNECTION_REPORT;
                reporter_print( reporthdr->multireport->report, CONNECTION_REPORT,
                                (reporthdr->report.type == 0 ? 1 : 0) );
            }
        }
    } else if ( (reporthdr->report.type & SERVER_RELAY_REPORT) != 0 ) {
        reporthdr->report.type &= ~SERVER_RELAY_REPORT;
        return reporter_print( &reporthdr->report, SERVER_RELAY_REPORT, 1 );
    }
    if ( (reporthdr->report.type & TRANSFER_REPORT) != 0 ) {
        // If there are more packets to process then handle them
        if ( reporthdr->reporterindex >= 0 ) {
            // Need to make sure we do not pass the "agent"
            while ( reporthdr->reporterindex != reporthdr->agentindex - 1 ) {
                if ( reporthdr->reporterindex == NUM_REPORT_STRUCTS - 1 ) {
                    if ( reporthdr->agentindex == 0 ) {
                        break;
                    } else {
                        reporthdr->reporterindex = 0;
                    }
                } else {
                    reporthdr->reporterindex++;
                }
                if ( reporter_handle_packet( reporthdr ) ) {
                    // No more packets to process
                    reporthdr->reporterindex = -1;
                    break;
                }
            }
        }
        // If the agent is done with the report then free it
        if ( reporthdr->agentindex == -1 ) {
            need_free = 1;
        }
    }
    return need_free;
}

/*
 * Updates connection stats
 */
int reporter_handle_packet( ReportHeader *reporthdr ) {
    ReportStruct *packet = &reporthdr->data[reporthdr->reporterindex];
    ReporterData *data = &reporthdr->report;
    Transfer_Info *stats = &reporthdr->report.info;
    int finished = 0;

    data->cntDatagrams++;
    // If this is the last packet set the endTime
    if ( packet->packetID < 0 ) {
        data->packetTime = packet->packetTime;
        finished = 1;
        if ( reporthdr->report.mThreadMode != kMode_Client ) {
            data->TotalLen += packet->packetLen;
        }
    } else {
        // update recieved amount and time
        data->packetTime = packet->packetTime;
        reporter_condprintstats( &reporthdr->report, reporthdr->multireport, finished );
        data->TotalLen += packet->packetLen;
        if ( packet->packetID != 0 ) {
            // UDP packet
            double transit;
            double deltaTransit;
            
            // from RFC 1889, Real Time Protocol (RTP) 
            // J = J + ( | D(i-1,i) | - J ) / 16 
            transit = TimeDifference( packet->packetTime, packet->sentTime );
            if ( data->lastTransit != 0.0 ) {
                deltaTransit = transit - data->lastTransit;
                if ( deltaTransit < 0.0 ) {
                    deltaTransit = -deltaTransit;
                }
                stats->jitter += (deltaTransit - stats->jitter) / (16.0);
            }
            data->lastTransit = transit;
    
            // packet loss occured if the datagram numbers aren't sequential 
            if ( packet->packetID != data->PacketID + 1 ) {
                if ( packet->packetID < data->PacketID + 1 ) {
                    data->cntOutofOrder++;
                } else {
                    data->cntError += packet->packetID - data->PacketID - 1;
                }
            }
            // never decrease datagramID (e.g. if we get an out-of-order packet) 
            if ( packet->packetID > data->PacketID ) {
                data->PacketID = packet->packetID;
            }
        }
    }

    // Print a report if appropriate
    return reporter_condprintstats( &reporthdr->report, reporthdr->multireport, finished );
}

/*
 * Handles summing of threads
 */
void reporter_handle_multiple_reports( MultiHeader *reporthdr, Transfer_Info *stats, int force ) {
    if ( reporthdr != NULL ) {
        if ( reporthdr->threads > 1 ) {
            int i;
            Transfer_Info *current = NULL;
            // Search for start Time
            for ( i = 0; i < NUM_MULTI_SLOTS; i++ ) {
                current = &reporthdr->data[i];
                if ( current->startTime == stats->startTime ) {
                    break;
                }
            }
            if ( current->startTime != stats->startTime ) {
                // Find first available
                for ( i = 0; i < NUM_MULTI_SLOTS; i++ ) {
                    current = &reporthdr->data[i];
                    if ( current->startTime < 0 ) {
                        break;
                    }
                }
                current->cntDatagrams = stats->cntDatagrams;
                current->cntError = stats->cntError;
                current->cntOutofOrder = stats->cntOutofOrder;
                current->TotalLen = stats->TotalLen;
                current->mFormat = stats->mFormat;
                current->endTime = stats->endTime;
                current->jitter = stats->jitter;
                current->startTime = stats->startTime;
                current->free = 1;
            } else {
                current->cntDatagrams += stats->cntDatagrams;
                current->cntError += stats->cntError;
                current->cntOutofOrder += stats->cntOutofOrder;
                current->TotalLen += stats->TotalLen;
                current->mFormat = stats->mFormat;
                if ( current->endTime < stats->endTime ) {
                    current->endTime = stats->endTime;
                }
                if ( current->jitter < stats->jitter ) {
                    current->jitter = stats->jitter;
                }
                current->free++;
                if ( current->free == reporthdr->threads ) {
                    void *reserved = reporthdr->report->info.reserved_delay;
                    current->free = force;
                    memcpy( &reporthdr->report->info, current, sizeof(Transfer_Info) );
                    current->startTime = -1;
                    reporthdr->report->info.reserved_delay = reserved;
                    reporter_print( reporthdr->report, MULTIPLE_REPORT, force );
                }
            }
        }
    }
}

/*
 * Prints reports conditionally
 */
int reporter_condprintstats( ReporterData *stats, MultiHeader *multireport, int force ) {
    if ( force != 0 ) {
        stats->info.cntOutofOrder = stats->cntOutofOrder;
        // assume most of the time out-of-order packets are not
        // duplicate packets, so conditionally subtract them from the lost packets.
        stats->info.cntError = stats->cntError;
        if ( stats->info.cntError > stats->info.cntOutofOrder ) {
            stats->info.cntError -= stats->info.cntOutofOrder;
        }
        stats->info.cntDatagrams = (isUDP(stats) ? stats->PacketID : stats->cntDatagrams);
        stats->info.TotalLen = stats->TotalLen;
        stats->info.startTime = 0;
        stats->info.endTime = TimeDifference( stats->packetTime, stats->startTime );
        stats->info.free = 1;
        reporter_print( stats, TRANSFER_REPORT, force );
        if ( isMultipleReport(stats) ) {
            reporter_handle_multiple_reports( multireport, &stats->info, force );
        }
    } else while ((stats->intervalTime.tv_sec != 0 || 
                   stats->intervalTime.tv_usec != 0) && 
                  TimeDifference( stats->nextTime, 
                                  stats->packetTime ) < 0 ) {
        stats->info.cntOutofOrder = stats->cntOutofOrder - stats->lastOutofOrder;
        stats->lastOutofOrder = stats->cntOutofOrder;
        // assume most of the time out-of-order packets are not
        // duplicate packets, so conditionally subtract them from the lost packets.
        stats->info.cntError = stats->cntError - stats->lastError;
        if ( stats->info.cntError > stats->info.cntOutofOrder ) {
            stats->info.cntError -= stats->info.cntOutofOrder;
        }
        stats->lastError = stats->cntError;
        stats->info.cntDatagrams = (isUDP( stats ) ? stats->PacketID - stats->lastDatagrams :
                                                     stats->cntDatagrams - stats->lastDatagrams);
        stats->lastDatagrams = (isUDP( stats ) ? stats->PacketID : stats->cntDatagrams);
        stats->info.TotalLen = stats->TotalLen - stats->lastTotal;
        stats->lastTotal = stats->TotalLen;
        stats->info.startTime = stats->info.endTime;
        stats->info.endTime = TimeDifference( stats->nextTime, stats->startTime );
        TimeAdd( stats->nextTime, stats->intervalTime );
        stats->info.free = 0;
        reporter_print( stats, TRANSFER_REPORT, force );
        if ( isMultipleReport(stats) ) {
            reporter_handle_multiple_reports( multireport, &stats->info, force );
        }
    }
    return force;
}

/*
 * This function handles multiple format printing by sending to the
 * appropriate dispatch function
 */
int reporter_print( ReporterData *stats, int type, int end ) {
    switch ( type ) {
        case TRANSFER_REPORT:
            statistics_reports[stats->mode]( &stats->info );
            if ( end != 0 && isPrintMSS( stats ) && !isUDP( stats ) ) {
                PrintMSS( stats );
            }
            break;
        case SERVER_RELAY_REPORT:
            serverstatistics_reports[stats->mode]( &stats->connection, &stats->info );
            break;
        case SETTINGS_REPORT:
            settings_reports[stats->mode]( stats );
            break;
        case CONNECTION_REPORT:
            stats->info.reserved_delay = connection_reports[stats->mode]( 
                                               &stats->connection,
                                               stats->info.transferID );
            break;
        case MULTIPLE_REPORT:
            multiple_reports[stats->mode]( &stats->info );
            break;
        default:
            fprintf( stderr, "Printing type not implemented! No Output\n" );
    }
    fflush( stdout );
    return end;
}

/* -------------------------------------------------------------------
 * Report the MSS and MTU, given the MSS (or a guess thereof)
 * ------------------------------------------------------------------- */

// compare the MSS against the (MTU - 40) to (MTU - 80) bytes.
// 40 byte IP header and somewhat arbitrarily, 40 more bytes of IP options.

#define checkMSS_MTU( inMSS, inMTU ) (inMTU-40) >= inMSS  &&  inMSS >= (inMTU-80)

void PrintMSS( ReporterData *stats ) {
    int inMSS = getsock_tcp_mss( stats->info.transferID );

    if ( inMSS <= 0 ) {
        printf( report_mss_unsupported, stats->info.transferID );
    } else {
        char* net;
        int mtu = 0;

        if ( checkMSS_MTU( inMSS, 1500 ) ) {
            net = "ethernet";
            mtu = 1500;
        } else if ( checkMSS_MTU( inMSS, 4352 ) ) {
            net = "FDDI";
            mtu = 4352;
        } else if ( checkMSS_MTU( inMSS, 9180 ) ) {
            net = "ATM";
            mtu = 9180;
        } else if ( checkMSS_MTU( inMSS, 65280 ) ) {
            net = "HIPPI";
            mtu = 65280;
        } else if ( checkMSS_MTU( inMSS, 576 ) ) {
            net = "minimum";
            mtu = 576;
            printf( warn_no_pathmtu );
        } else {
            mtu = inMSS + 40;
            net = "unknown interface";
        }

        printf( report_mss,
                stats->info.transferID, inMSS, mtu, net );
    }
}
// end ReportMSS

#ifdef __cplusplus
} /* end extern "C" */
#endif
