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
 * Locale.c
 * by Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * & Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * Strings and other stuff that is locale specific.
 * ------------------------------------------------------------------- */

#include "version.h"

#ifdef __cplusplus
extern    "C"
{
#endif


/* -------------------------------------------------------------------
 * usage
 * ------------------------------------------------------------------- */

const char usage_shortstr[] = "Usage: iperf [-s|-c host] [options]\n"
                           "Try `iperf --help' for more information.\n";

const char usage_longstr[] = "Usage: iperf [-s|-c host] [options]\n"
                           "       iperf [-h|--help] [-v|--version]\n\n"
                           "Server or Client:\n"
                           "  -p, --port      #         server port to listen on/connect to\n"
                           "  -f, --format    [kmgKMG]  format to report: Kbits, Mbits, KBytes, MBytes\n"
                           "  -i, --interval  #         seconds between periodic bandwidth reports\n"
                           "  -F, --file name           xmit/recv the specified file\n"
#if defined(linux) || defined(__FreeBSD__)
                           "  -A, --affinity n/n,m      set CPU affinity\n"
#endif
                           "  -V, --verbose             more detailed output\n"
                           "  -J, --json                output in JSON format\n"
                           "  -d, --debug               emit debugging output\n"
                           "  -v, --version             show version information and quit\n"
                           "  -h, --help                show this message and quit\n"
                           "Server specific:\n"
                           "  -s, --server              run in server mode\n"
                           "  -D, --daemon              run the server as a daemon\n"
                           "  -I, --pidfile file        write PID file\n"
                           "Client specific:\n"
                           "  -c, --client    <host>    run in client mode, connecting to <host>\n"
#if defined(linux) || defined(__FreeBSD__)
                           "  --sctp                    use SCTP rather than TCP\n"
#endif
                           "  -u, --udp                 use UDP rather than TCP\n"
                           "  -b, --bandwidth #[KMG][/#] target bandwidth in bits/sec\n"
                           "                            (default %d Mbit/sec for UDP, unlimited for TCP)\n"
                           "                            (optional slash and packet count for burst mode)\n"
                           "  -t, --time      #         time in seconds to transmit for (default %d secs)\n"
                           "  -n, --num       #[KMG]    number of bytes to transmit (instead of -t)\n"
                           "  -k, --blockcount #[KMG]   number of blocks (packets) to transmit (instead of -t or -n)\n"
                           "  -l, --len       #[KMG]    length of buffer to read or write\n"
			   "                            (default %d KB for TCP, %d KB for UDP)\n"
                           "  -P, --parallel  #         number of parallel client streams to run\n"
                           "  -R, --reverse             run in reverse mode (server sends, client receives)\n"
                           "  -w, --window    #[KMG]    TCP window size (socket buffer size)\n"
                           "  -B, --bind      <host>    bind to a specific interface\n"
#if defined(linux)
                           "  -C, --linux-congestion <algo>  set TCP congestion control algorithm (Linux only)\n"
#endif
                           "  -M, --set-mss   #         set TCP maximum segment size (MTU - 40 bytes)\n"
                           "  -N, --nodelay             set TCP no delay, disabling Nagle's Algorithm\n"
                           "  -4, --version4            only use IPv4\n"
                           "  -6, --version6            only use IPv6\n"
                           "  -S, --tos N               set the IP 'type of service'\n"
#if defined(linux)
                           "  -L, --flowlabel N         set the IPv6 flow label (only supported on Linux)\n"
#endif
                           "  -Z, --zerocopy            use a 'zero copy' method of sending data\n"
                           "  -O, --omit N              omit the first n seconds\n"
                           "  -T, --title str           prefix every output line with this string\n"

#ifdef NOT_YET_SUPPORTED /* still working on these */
#endif

			   "\n"
                           "[KMG] indicates options that support a K/M/G suffix for kilo-, mega-, or giga-\n"
                           "Report bugs to <iperf-users@lists.sourceforge.net>\n";


#ifdef OBSOLETE /* from old iperf: no longer supported. Add some of these back someday */
  "-d, --dualtest           Do a bidirectional test simultaneously\n"
  "-L, --listenport #       port to recieve bidirectional tests back on\n"
  "-I, --stdin              input the data to be transmitted from stdin\n"
  "-F, --fileinput <name>   input the data to be transmitted from a file\n"
  "-r, --tradeoff           Do a bidirectional test individually\n"
  "-T, --ttl       #        time-to-live, for multicast (default 1)\n"
  "-x, --reportexclude [CDMSV]   exclude C(connection) D(data) M(multicast) S(settings) V(server) reports\n"
  "-y, --reportstyle C      report as a Comma-Separated Values\n"
#endif

const char version[] = "iperf version " IPERF_VERSION " (" IPERF_VERSION_DATE ")";

/* -------------------------------------------------------------------
 * settings
 * ------------------------------------------------------------------- */

const char seperator_line[] =
"------------------------------------------------------------\n";

const char server_port[] =
"Server listening on %s port %d\n";

const char client_port[] =
"Client connecting to %s, %s port %d\n";

const char bind_address[] =
"Binding to local address %s\n";

const char multicast_ttl[] =
"Setting multicast TTL to %d\n";

const char join_multicast[] =
"Joining multicast group  %s\n";

const char client_datagram_size[] =
"Sending %d byte datagrams\n";

const char server_datagram_size[] =
"Receiving %d byte datagrams\n";

const char tcp_window_size[] =
"TCP window size";

const char udp_buffer_size[] =
"UDP buffer size";

const char window_default[] =
"(default)";

const char wait_server_threads[] =
"Waiting for server threads to complete. Interrupt again to force quit.\n";

const char test_start_time[] =
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %d second test\n";

const char test_start_bytes[] =
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %llu bytes to send\n";

const char test_start_blocks[] =
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %d blocks to send\n";


/* -------------------------------------------------------------------
 * reports
 * ------------------------------------------------------------------- */

const char report_time[] =
"Time: %s\n";

const char report_connecting[] =
"Connecting to host %s, port %d\n";

const char report_reverse[] =
"Reverse mode, remote host %s is sending\n";

const char report_accepted[] =
"Accepted connection from %s, port %d\n";

const char report_cookie[] =
"      Cookie: %s\n";

const char report_connected[] =
"[%3d] local %s port %d connected to %s port %d\n";

const char report_window[] =
"TCP window size: %s\n";

const char report_autotune[] =
"Using TCP Autotuning\n";

const char report_omit_done[] =
"Finished omit period, starting real test\n";

const char report_diskfile[] =
"        Sent %s / %s (%d%%) of %s\n";

const char report_done[] =
"iperf Done.\n";

const char report_read_lengths[] =
"[%3d] Read lengths occurring in more than 5%% of reads:\n";

const char report_read_length_times[] =
"[%3d] %5d bytes read %5d times (%.3g%%)\n";

const char report_bw_header[] =
"[ ID] Interval           Transfer     Bandwidth\n";

const char report_bw_retrans_header[] =
"[ ID] Interval           Transfer     Bandwidth       Retr\n";

const char report_bw_retrans_cwnd_header[] =
"[ ID] Interval           Transfer     Bandwidth       Retr  Cwnd\n";

const char report_bw_udp_header[] =
"[ ID] Interval           Transfer     Bandwidth       Jitter    Lost/Total Datagrams\n";

const char report_bw_udp_sender_header[] =
"[ ID] Interval           Transfer     Bandwidth       Total Datagrams\n";

const char report_bw_format[] =
"[%3d] %6.2f-%-6.2f sec  %ss  %ss/sec                  %s\n";

const char report_bw_retrans_format[] =
"[%3d] %6.2f-%-6.2f sec  %ss  %ss/sec  %3u             %s\n";

const char report_bw_retrans_cwnd_format[] =
"[%3d] %6.2f-%-6.2f sec  %ss  %ss/sec  %3u   %ss       %s\n";

const char report_bw_udp_format[] =
"[%3d] %6.2f-%-6.2f sec  %ss  %ss/sec  %5.3f ms  %d/%d (%.2g%%)  %s\n";

const char report_bw_udp_sender_format[] =
"[%3d] %6.2f-%-6.2f sec  %ss  %ss/sec  %d  %s\n";

const char report_summary[] =
"Test Complete. Summary Results:\n";

const char report_sum_bw_format[] =
"[SUM] %6.2f-%-6.2f sec  %ss  %ss/sec                  %s\n";

const char report_sum_bw_retrans_format[] =
"[SUM] %6.2f-%-6.2f sec  %ss  %ss/sec  %3d             %s\n";

const char report_sum_bw_udp_format[] =
"[SUM] %6.2f-%-6.2f sec  %ss  %ss/sec  %5.3f ms  %d/%d (%.2g%%)  %s\n";

const char report_sum_bw_udp_sender_format[] =
"[SUM] %6.2f-%-6.2f sec  %ss  %ss/sec  %d  %s\n";

const char report_omitted[] = "(omitted)";

const char report_bw_separator[] =
"- - - - - - - - - - - - - - - - - - - - - - - - -\n";

const char report_outoforder[] =
"[%3d] %4.1f-%4.1f sec  %d datagrams received out-of-order\n";

const char report_sum_outoforder[] =
"[SUM] %4.1f-%4.1f sec  %d datagrams received out-of-order\n";

const char report_peer[] =
"[%3d] local %s port %u connected with %s port %u\n";

const char report_mss_unsupported[] =
"[%3d] MSS and MTU size unknown (TCP_MAXSEG not supported by OS?)\n";

const char report_mss[] =
"[%3d] MSS size %d bytes (MTU %d bytes, %s)\n";

const char report_datagrams[] =
"[%3d] Sent %d datagrams\n";

const char report_sum_datagrams[] =
"[SUM] Sent %d datagrams\n";

const char server_reporting[] =
"[%3d] Server Report:\n";

const char reportCSV_peer[] =
"%s,%u,%s,%u";

const char report_cpu[] =
"CPU Utilization: %s/%s %.1f%% (%.1f%%u/%.1f%%s), %s/%s %.1f%% (%.1f%%u/%.1f%%s)\n";

const char report_local[] = "local";
const char report_remote[] = "remote";
const char report_sender[] = "sender";
const char report_receiver[] = "receiver";

#if defined(linux)
const char report_tcpInfo[] =
"event=TCP_Info CWND=%u SND_SSTHRESH=%u RCV_SSTHRESH=%u UNACKED=%u SACK=%u LOST=%u RETRANS=%u FACK=%u RTT=%u REORDERING=%u\n";
#endif
#if defined(__FreeBSD__)
const char report_tcpInfo[] =
"event=TCP_Info CWND=%u RCV_WIND=%u SND_SSTHRESH=%u RTT=%u\n";
#endif


#ifdef HAVE_QUAD_SUPPORT
#ifdef HAVE_PRINTF_QD
const char reportCSV_bw_format[] =
"%s,%s,%d,%.1f-%.1f,%qd,%qd\n";

const char reportCSV_bw_udp_format[] =
"%s,%s,%d,%.1f-%.1f,%qd,%qd,%.3f,%d,%d,%.3f,%d\n";
#else // HAVE_PRINTF_QD
const char reportCSV_bw_format[] =
"%s,%s,%d,%.1f-%.1f,%lld,%lld\n";

const char reportCSV_bw_udp_format[] =
"%s,%s,%d,%.1f-%.1f,%lld,%lld,%.3f,%d,%d,%.3f,%d\n";
#endif // HAVE_PRINTF_QD
#else // HAVE_QUAD_SUPPORT
#ifdef WIN32
const char reportCSV_bw_format[] =
"%s,%s,%d,%.1f-%.1f,%I64d,%I64d\n";

const char reportCSV_bw_udp_format[] =
"%s,%s,%d,%.1f-%.1f,%I64d,%I64d,%.3f,%d,%d,%.3f,%d\n";
#else
const char reportCSV_bw_format[] =
"%s,%s,%d,%.1f-%.1f,%d,%d\n";

const char reportCSV_bw_udp_format[] =
"%s,%s,%d,%.1f-%.1f,%d,%d,%.3f,%d,%d,%.3f,%d\n";
#endif //WIN32
#endif //HAVE_QUAD_SUPPORT
/* -------------------------------------------------------------------
 * warnings
 * ------------------------------------------------------------------- */

const char warn_window_requested[] =
" (WARNING: requested %s)";

const char warn_window_small[] =
"WARNING: TCP window size set to %d bytes. A small window size\n\
will give poor performance. See the Iperf documentation.\n";

const char warn_delay_large[] =
"WARNING: delay too large, reducing from %.1f to 1.0 seconds.\n";

const char warn_no_pathmtu[] =
"WARNING: Path MTU Discovery may not be enabled.\n";

const char warn_no_ack[]=
"[%3d] WARNING: did not receive ack of last datagram after %d tries.\n";

const char warn_ack_failed[]=
"[%3d] WARNING: ack of last datagram failed after %d tries.\n";

const char warn_fileopen_failed[]=
"WARNING: Unable to open file stream for transfer\n\
Using default data stream. \n";

const char unable_to_change_win[]=
"WARNING: Unable to change the window size\n";

const char opt_estimate[]=
"Optimal Estimate\n";

const char report_interval_small[] =
"WARNING: interval too small, increasing from %3.2f to 0.5 seconds.\n";

const char warn_invalid_server_option[] =
"WARNING: option -%c is not valid for server mode\n";

const char warn_invalid_client_option[] =
"WARNING: option -%c is not valid for client mode\n";

const char warn_invalid_compatibility_option[] =
"WARNING: option -%c is not valid in compatibility mode\n";

const char warn_implied_udp[] =
"WARNING: option -%c implies udp testing\n";

const char warn_implied_compatibility[] =
"WARNING: option -%c has implied compatibility mode\n";

const char warn_buffer_too_small[] =
"WARNING: the UDP buffer was increased to %d for proper operation\n";

const char warn_invalid_single_threaded[] =
"WARNING: option -%c is not valid in single threaded versions\n";

const char warn_invalid_report_style[] =
"WARNING: unknown reporting style \"%s\", switching to default\n";

const char warn_invalid_report[] =
"WARNING: unknown reporting type \"%c\", ignored\n valid options are:\n\t exclude: C(connection) D(data) M(multicast) S(settings) V(server) report\n\n";

#ifdef __cplusplus
} /* end extern "C" */
#endif
