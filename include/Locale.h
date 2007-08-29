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
 * Locale.h
 * by Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * & Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * Strings and other stuff that is locale specific.
 * ------------------------------------------------------------------- */

#ifndef LOCALE_H
#define LOCALE_H

#ifdef __cplusplus
extern "C" {
#endif
/* -------------------------------------------------------------------
 * usage
 * ------------------------------------------------------------------- */

extern const char usage_short[];

#ifdef WIN32
extern const char usage_long1[];
extern const char usage_long2[];
#else
extern const char usage_long[];
#endif

extern const char version[];

/* -------------------------------------------------------------------
 * settings
 * ------------------------------------------------------------------- */

extern const char seperator_line[];

extern const char server_port[];

extern const char client_port[];

extern const char bind_address[];

extern const char multicast_ttl[];

extern const char join_multicast[];

extern const char client_datagram_size[];

extern const char server_datagram_size[];

extern const char tcp_window_size[];

extern const char udp_buffer_size[];

extern const char window_default[];

extern const char wait_server_threads[];

/* -------------------------------------------------------------------
 * reports
 * ------------------------------------------------------------------- */

extern const char report_read_lengths[];

extern const char report_read_length_times[];

extern const char report_bw_header[];

extern const char report_bw_format[];

extern const char report_sum_bw_format[];

extern const char report_bw_jitter_loss_header[];

extern const char report_bw_jitter_loss_format[];

extern const char report_sum_bw_jitter_loss_format[];

extern const char report_outoforder[];

extern const char report_sum_outoforder[];

extern const char report_peer[];

extern const char report_mss_unsupported[];

extern const char report_mss[];

extern const char report_datagrams[];

extern const char report_sum_datagrams[];

extern const char server_reporting[];

extern const char reportCSV_peer[];

extern const char reportCSV_bw_format[];

extern const char reportCSV_bw_jitter_loss_format[];

/* -------------------------------------------------------------------
 * warnings
 * ------------------------------------------------------------------- */

extern const char warn_window_requested[];

extern const char warn_window_small[];

extern const char warn_delay_large[];

extern const char warn_no_pathmtu[];

extern const char warn_no_ack[];

extern const char warn_ack_failed[];

extern const char warn_fileopen_failed[];

extern const char unable_to_change_win[];

extern const char opt_estimate[];

extern const char report_interval_small[];

extern const char warn_invalid_server_option[];

extern const char warn_invalid_client_option[];

extern const char warn_invalid_compatibility_option[];

extern const char warn_implied_udp[];

extern const char warn_implied_compatibility[];

extern const char warn_buffer_too_small[];

extern const char warn_invalid_single_threaded[];

extern const char warn_invalid_report_style[];

extern const char warn_invalid_report[];

#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif // LOCALE_H










