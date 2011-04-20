/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef        IPERF_LOCALE_H
#define        IPERF_LOCALE_H

extern char usage_short[];
extern char usage_long1[];
extern char usage_long2[];
extern char version[];

extern char seperator_line[];

extern char server_port[] ;
extern char client_port[] ;
extern char bind_address[] ;
extern char multicast_ttl[] ;
extern char join_multicast[] ;
extern char client_datagram_size[] ;
extern char server_datagram_size[] ;
extern char tcp_window_size[] ;
extern char udp_buffer_size[] ;
extern char window_default[] ;
extern char wait_server_threads[] ;
extern char test_start_time[];
extern char test_start_bytes[];

extern char report_read_lengths[] ;
extern char report_read_length_times[] ;
extern char report_bw_header[] ;
extern char report_bw_format[] ;
extern char report_sum_bw_format[] ;
extern char report_bw_jitter_loss_header[] ;
extern char report_bw_jitter_loss_format[] ;
extern char report_sum_bw_jitter_loss_format[] ;
extern char report_outoforder[] ;
extern char report_sum_outoforder[] ;
extern char report_peer[] ;
extern char report_mss_unsupported[] ;
extern char report_mss[] ;
extern char report_datagrams[] ;
extern char report_sum_datagrams[] ;
extern char server_reporting[] ;
extern char reportCSV_peer[] ;

extern char report_tcpInfo[] ;
extern char report_tcpInfo[] ;


extern char warn_window_requested[] ;
extern char warn_window_small[] ;
extern char warn_delay_large[] ;
extern char warn_no_pathmtu[] ;
extern char warn_no_ack[];
extern char warn_ack_failed[];
extern char warn_fileopen_failed[];
extern char unable_to_change_win[];
extern char opt_estimate[];
extern char report_interval_small[] ;
extern char warn_invalid_server_option[] ;
extern char warn_invalid_client_option[] ;
extern char warn_invalid_compatibility_option[] ;
extern char warn_implied_udp[] ;
extern char warn_implied_compatibility[] ;
extern char warn_buffer_too_small[] ;
extern char warn_invalid_single_threaded[] ;
extern char warn_invalid_report_style[] ;
extern char warn_invalid_report[] ;

#endif
