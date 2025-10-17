/*
 * iperf, Copyright (c) 2014-2020, The Regents of the University of
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
 */
#ifndef        IPERF_LOCALE_H
#define        IPERF_LOCALE_H

extern const char usage_shortstr[];
extern const char usage_longstr[];
extern const char version[];

extern const char seperator_line[];

extern const char server_port[] ;
extern const char client_port[] ;
extern const char bind_address[] ;
extern const char bind_dev[] ;
extern const char multicast_ttl[] ;
extern const char join_multicast[] ;
extern const char client_datagram_size[] ;
extern const char server_datagram_size[] ;
extern const char tcp_window_size[] ;
extern const char udp_buffer_size[] ;
extern const char window_default[] ;
extern const char wait_server_threads[] ;
extern const char test_start_time[];
extern const char test_start_bytes[];
extern const char test_start_blocks[];

extern const char report_time[] ;
extern const char report_connecting[] ;
extern const char report_reverse[] ;
extern const char report_accepted[] ;
extern const char report_cookie[] ;
extern const char report_connected[] ;
extern const char report_authentication_succeeded[] ;
extern const char report_authentication_failed[] ;
extern const char report_window[] ;
extern const char report_autotune[] ;
extern const char report_omit_done[] ;
extern const char report_diskfile[] ;
extern const char report_done[] ;
extern const char report_read_lengths[] ;
extern const char report_read_length_times[] ;
extern const char report_bw_header[] ;
extern const char report_bw_header_bidir[] ;
extern const char report_bw_retrans_header[] ;
extern const char report_bw_retrans_header_bidir[] ;
extern const char report_bw_retrans_cwnd_header[] ;
extern const char report_bw_retrans_cwnd_header_bidir[] ;
extern const char report_bw_udp_header[] ;
extern const char report_bw_udp_header_bidir[] ;
extern const char report_bw_udp_sender_header[] ;
extern const char report_bw_udp_sender_header_bidir[] ;
extern const char report_bw_format[] ;
extern const char report_bw_retrans_format[] ;
extern const char report_bw_retrans_cwnd_format[] ;
extern const char report_bw_udp_format[] ;
extern const char report_bw_udp_format_no_omitted_error[] ;
extern const char report_bw_udp_sender_format[] ;
extern const char report_summary[] ;
extern const char report_sum_bw_format[] ;
extern const char report_sum_bw_retrans_format[] ;
extern const char report_sum_bw_udp_format[] ;
extern const char report_sum_bw_udp_sender_format[] ;
extern const char report_omitted[] ;
extern const char report_bw_separator[] ;
extern const char report_outoforder[] ;
extern const char report_sum_outoforder[] ;
extern const char report_peer[] ;
extern const char report_mss_unsupported[] ;
extern const char report_mss[] ;
extern const char report_datagrams[] ;
extern const char report_sum_datagrams[] ;
extern const char server_reporting[] ;
extern const char reportCSV_peer[] ;

extern const char report_cpu[] ;
extern const char report_local[] ;
extern const char report_remote[] ;
extern const char report_sender[] ;
extern const char report_receiver[] ;
extern const char report_sender_not_available_format[];
extern const char report_sender_not_available_summary_format[];
extern const char report_receiver_not_available_format[];
extern const char report_receiver_not_available_summary_format[];

extern const char report_tcpInfo[] ;
extern const char report_tcpInfo[] ;


extern const char warn_window_requested[] ;
extern const char warn_window_small[] ;
extern const char warn_delay_large[] ;
extern const char warn_no_pathmtu[] ;
extern const char warn_no_ack[];
extern const char warn_ack_failed[];
extern const char warn_fileopen_failed[];
extern const char unable_to_change_win[];
extern const char opt_estimate[];
extern const char report_interval_small[] ;
extern const char warn_invalid_server_option[] ;
extern const char warn_invalid_client_option[] ;
extern const char warn_invalid_compatibility_option[] ;
extern const char warn_implied_udp[] ;
extern const char warn_implied_compatibility[] ;
extern const char warn_buffer_too_small[] ;
extern const char warn_invalid_single_threaded[] ;
extern const char warn_invalid_report_style[] ;
extern const char warn_invalid_report[] ;

#endif
