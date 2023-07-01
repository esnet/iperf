/*---------------------------------------------------------------
 * iperf, Copyright (c) 2014-2022, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 *
 * Based on code that is:
 *
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
#include "iperf_config.h"

#include "version.h"

#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#else
# ifndef PRIu64
#  if sizeof(long) == 8
#   define PRIu64		"lu"
#  else
#   define PRIu64		"llu"
#  endif
# endif
#endif

#ifdef __cplusplus
extern    "C"
{
#endif


/* -------------------------------------------------------------------
 * usage
 * ------------------------------------------------------------------- */

const char usage_shortstr[] = "Usage: iperf3 [-s|-c host] [options]\n"
                           "Try `iperf3 --help' for more information.\n";

const char usage_longstr[] = "Usage: iperf3 [-s|-c host] [options]\n"
                           "       iperf3 [-h|--help] [-v|--version]\n\n"
                           "Server or Client:\n"
                           "  -p, --port      #         server port to listen on/connect to\n"
                           "  -f, --format   [kmgtKMGT] format to report: Kbits, Mbits, Gbits, Tbits\n"
                           "  -i, --interval  #         seconds between periodic throughput reports\n"
                           "  -I, --pidfile file        write PID file\n"
                           "  -F, --file name           xmit/recv the specified file\n"
#if defined(HAVE_CPU_AFFINITY)
                           "  -A, --affinity n/n,m      set CPU affinity\n"
#endif /* HAVE_CPU_AFFINITY */
#if defined(HAVE_SO_BINDTODEVICE)
                           "  -B, --bind <host>[%%<dev>] bind to the interface associated with the address <host>\n"
                           "                            (optional <dev> equivalent to `--bind-dev <dev>`)\n"
                           "  --bind-dev <dev>          bind to the network interface with SO_BINDTODEVICE\n"
#else /* HAVE_SO_BINDTODEVICE */
                           "  -B, --bind      <host>    bind to the interface associated with the address <host>\n"
#endif /* HAVE_SO_BINDTODEVICE */
                           "  -V, --verbose             more detailed output\n"
                           "  -J, --json                output in JSON format\n"
                           "  --logfile f               send output to a log file\n"
                           "  --forceflush              force flushing output at every interval\n"
                           "  --timestamps<=format>     emit a timestamp at the start of each output line\n"
                           "                            (optional \"=\" and format string as per strftime(3))\n"

                           "  --rcv-timeout #           idle timeout for receiving data (default %d ms)\n"
#if defined(HAVE_TCP_USER_TIMEOUT)
                           "  --snd-timeout #           timeout for unacknowledged TCP data\n"
                           "                            (in ms, default is system settings)\n"
#endif /* HAVE_TCP_USER_TIMEOUT */
                           "  -d, --debug[=#]           emit debugging output\n"
                           "                            (optional optional \"=\" and debug level: 1-4. Default is 4 - all messages)\n"
                           "  -v, --version             show version information and quit\n"
                           "  -h, --help                show this message and quit\n"
                           "Server specific:\n"
                           "  -s, --server              run in server mode\n"
                           "  -D, --daemon              run the server as a daemon\n"
                           "  -1, --one-off             handle one client connection then exit\n"
			   "  --server-bitrate-limit #[KMG][/#]   server's total bit rate limit (default 0 = no limit)\n"
			   "                            (optional slash and number of secs interval for averaging\n"
			   "                            total data rate.  Default is 5 seconds)\n"
                           "  --idle-timeout #          restart idle server after # seconds in case it\n"
                           "                            got stuck (default - no timeout)\n"
#if defined(HAVE_SSL)
                           "  --rsa-private-key-path    path to the RSA private key used to decrypt\n"
			   "                            authentication credentials\n"
                           "  --authorized-users-path   path to the configuration file containing user\n"
                           "                            credentials\n"
                           "  --time-skew-threshold     time skew threshold (in seconds) between the server\n"
                           "                            and client during the authentication process\n"
#endif //HAVE_SSL
                           "Client specific:\n"
                           "  -c, --client <host>[%%<dev>] run in client mode, connecting to <host>\n"
                           "                              (option <dev> equivalent to `--bind-dev <dev>`)\n"
#if defined(HAVE_SCTP_H)
                           "  --sctp                    use SCTP rather than TCP\n"
                           "  -X, --xbind <name>        bind SCTP association to links\n"
                           "  --nstreams      #         number of SCTP streams\n"
#endif /* HAVE_SCTP_H */
                           "  -u, --udp                 use UDP rather than TCP\n"
                           "  --connect-timeout #       timeout for control connection setup (ms)\n"
                           "  -b, --bitrate #[KMG][/#]  target bitrate in bits/sec (0 for unlimited)\n"
                           "                            (default %d Mbit/sec for UDP, unlimited for TCP)\n"
                           "                            (optional slash and packet count for burst mode)\n"
			   "  --pacing-timer #[KMG]     set the timing for pacing, in microseconds (default %d)\n"
#if defined(HAVE_SO_MAX_PACING_RATE)
                           "  --fq-rate #[KMG]          enable fair-queuing based socket pacing in\n"
			   "                            bits/sec (Linux only)\n"
#endif
                           "  -t, --time      #         time in seconds to transmit for (default %d secs)\n"
                           "  -n, --bytes     #[KMG]    number of bytes to transmit (instead of -t)\n"
                           "  -k, --blockcount #[KMG]   number of blocks (packets) to transmit (instead of -t or -n)\n"
                           "  -l, --length    #[KMG]    length of buffer to read or write\n"
			   "                            (default %d KB for TCP, dynamic or %d for UDP)\n"
                           "  --cport         <port>    bind to a specific client port (TCP and UDP, default: ephemeral port)\n"
                           "  -P, --parallel  #         number of parallel client streams to run\n"
                           "  -R, --reverse             run in reverse mode (server sends, client receives)\n"
                           "  --bidir                   run in bidirectional mode.\n"
                           "                            Client and server send and receive data.\n"
                           "  -w, --window    #[KMG]    set send/receive socket buffer sizes\n"
                           "                            (indirectly sets TCP window size)\n"

#if defined(HAVE_TCP_CONGESTION)
                           "  -C, --congestion <algo>   set TCP congestion control algorithm (Linux and FreeBSD only)\n"
#endif /* HAVE_TCP_CONGESTION */
                           "  -M, --set-mss   #         set TCP/SCTP maximum segment size (MTU - 40 bytes)\n"
                           "  -N, --no-delay            set TCP/SCTP no delay, disabling Nagle's Algorithm\n"
                           "  -4, --version4            only use IPv4\n"
                           "  -6, --version6            only use IPv6\n"
                           "  -S, --tos N               set the IP type of service, 0-255.\n"
                           "                            The usual prefixes for octal and hex can be used,\n"
                           "                            i.e. 52, 064 and 0x34 all specify the same value.\n"

                           "  --dscp N or --dscp val    set the IP dscp value, either 0-63 or symbolic.\n"
                           "                            Numeric values can be specified in decimal,\n"
                           "                            octal and hex (see --tos above).\n"
#if defined(HAVE_FLOWLABEL)
                           "  -L, --flowlabel N         set the IPv6 flow label (only supported on Linux)\n"
#endif /* HAVE_FLOWLABEL */
                           "  -Z, --zerocopy            use a 'zero copy' method of sending data\n"
                           "  -O, --omit N              perform pre-test for N seconds and omit the pre-test statistics\n"
                           "  -T, --title str           prefix every output line with this string\n"
                           "  --extra-data str          data string to include in client and server JSON\n"
                           "  --get-server-output       get results from server\n"
                           "  --udp-counters-64bit      use 64-bit counters in UDP test packets\n"
                           "  --repeating-payload       use repeating pattern in payload, instead of\n"
                           "                            randomized payload (like in iperf2)\n"
#if defined(HAVE_DONT_FRAGMENT)
                           "  --dont-fragment           set IPv4 Don't Fragment flag\n"
#endif /* HAVE_DONT_FRAGMENT */
#if defined(HAVE_SSL)
                           "  --username                username for authentication\n"
                           "  --rsa-public-key-path     path to the RSA public key used to encrypt\n"
                           "                            authentication credentials\n"
#endif //HAVESSL

#ifdef NOT_YET_SUPPORTED /* still working on these */
#endif

			   "\n"
                           "[KMG] indicates options that support a K/M/G suffix for kilo-, mega-, or giga-\n"
			   "\n"
#ifdef PACKAGE_URL
                           "iperf3 homepage at: " PACKAGE_URL "\n"
#endif /* PACKAGE_URL */
#ifdef PACKAGE_BUGREPORT
                           "Report bugs to:     " PACKAGE_BUGREPORT "\n"
#endif /* PACKAGE_BUGREPORT */
			   ;

#ifdef OBSOLETE /* from old iperf: no longer supported. Add some of these back someday */
  "-d, --dualtest           Do a bidirectional test simultaneously\n"
  "-L, --listenport #       port to receive bidirectional tests back on\n"
  "-I, --stdin              input the data to be transmitted from stdin\n"
  "-F, --fileinput <name>   input the data to be transmitted from a file\n"
  "-r, --tradeoff           Do a bidirectional test individually\n"
  "-T, --ttl       #        time-to-live, for multicast (default 1)\n"
  "-x, --reportexclude [CDMSV]   exclude C(connection) D(data) M(multicast) S(settings) V(server) reports\n"
  "-y, --reportstyle C      report as a Comma-Separated Values\n"
#endif

const char version[] = PACKAGE_STRING;

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

const char bind_dev[] =
"Binding to local network device %s\n";

const char bind_port[] =
"Binding to local port %s\n";

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
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %d second test, tos %d\n";

const char test_start_bytes[] =
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %llu bytes to send, tos %d\n";

const char test_start_blocks[] =
"Starting Test: protocol: %s, %d streams, %d byte blocks, omitting %d seconds, %d blocks to send, tos %d\n";


/* -------------------------------------------------------------------
 * reports
 * ------------------------------------------------------------------- */

const char report_time[] =
"Time: %s\n";

const char report_connecting[] =
"Connecting to host %s, port %d\n";

const char report_authentication_succeeded[] =
"Authentication succeeded for user '%s' ts %ld\n";

const char report_authentication_failed[] =
"Authentication failed with return code %d for user '%s' ts %ld\n";

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
"[ ID] Interval           Transfer     Bitrate\n";

const char report_bw_header_bidir[] =
"[ ID][Role] Interval           Transfer     Bitrate\n";

const char report_bw_retrans_header[] =
"[ ID] Interval           Transfer     Bitrate         Retr\n";

const char report_bw_retrans_header_bidir[] =
"[ ID][Role] Interval           Transfer     Bitrate         Retr\n";

const char report_bw_retrans_cwnd_header[] =
"[ ID] Interval           Transfer     Bitrate         Retr  Cwnd\n";

const char report_bw_retrans_cwnd_header_bidir[] =
"[ ID][Role] Interval           Transfer     Bitrate         Retr  Cwnd\n";

const char report_bw_udp_header[] =
"[ ID] Interval           Transfer     Bitrate         Jitter    Lost/Total Datagrams\n";

const char report_bw_udp_header_bidir[] =
"[ ID][Role] Interval           Transfer     Bitrate         Jitter    Lost/Total Datagrams\n";

const char report_bw_udp_sender_header[] =
"[ ID] Interval           Transfer     Bitrate         Total Datagrams\n";

const char report_bw_udp_sender_header_bidir[] =
"[ ID][Role] Interval           Transfer     Bitrate         Total Datagrams\n";

const char report_bw_format[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec                  %s\n";

const char report_bw_retrans_format[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %3u             %s\n";

const char report_bw_retrans_cwnd_format[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %3u   %ss       %s\n";

const char report_bw_udp_format[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %5.3f ms  %" PRIu64 "/%" PRIu64 " (%.2g%%)  %s\n";

const char report_bw_udp_format_no_omitted_error[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %5.3f ms  Unknown/%" PRIu64 "  %s\n";

const char report_bw_udp_sender_format[] =
"[%3d]%s %6.2f-%-6.2f sec  %ss  %ss/sec %s %" PRIu64 "  %s\n";

const char report_summary[] =
"Test Complete. Summary Results:\n";

const char report_sum_bw_format[] =
"[SUM]%s %6.2f-%-6.2f sec  %ss  %ss/sec                  %s\n";

const char report_sum_bw_retrans_format[] =
"[SUM]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %3d             %s\n";

const char report_sum_bw_udp_format[] =
"[SUM]%s %6.2f-%-6.2f sec  %ss  %ss/sec  %5.3f ms  %" PRIu64 "/%" PRIu64 " (%.2g%%)  %s\n";

const char report_sum_bw_udp_sender_format[] =
"[SUM]%s %6.2f-%-6.2f sec  %ss  %ss/sec %s %" PRIu64 "  %s\n";

const char report_omitted[] = "(omitted)";

const char report_bw_separator[] =
"- - - - - - - - - - - - - - - - - - - - - - - - -\n";

const char report_outoforder[] =
"[%3d]%s %4.1f-%4.1f sec  %d datagrams received out-of-order\n";

const char report_sum_outoforder[] =
"[SUM]%s %4.1f-%4.1f sec  %d datagrams received out-of-order\n";

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
const char report_sender_not_available_format[] = "[%3d] (sender statistics not available)\n";
const char report_sender_not_available_summary_format[] = "[%3s] (sender statistics not available)\n";
const char report_receiver_not_available_format[] = "[%3d] (receiver statistics not available)\n";
const char report_receiver_not_available_summary_format[] = "[%3s] (receiver statistics not available)\n";

#if defined(linux)
const char report_tcpInfo[] =
"event=TCP_Info CWND=%u SND_SSTHRESH=%u RCV_SSTHRESH=%u UNACKED=%u SACK=%u LOST=%u RETRANS=%u FACK=%u RTT=%u REORDERING=%u\n";
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
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
