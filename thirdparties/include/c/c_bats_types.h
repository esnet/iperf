/**
 * @file c_bats_types.h
 * @author Lei Peng (peng.lei@n-hop.com)
 * @brief  BATS protocol c-style types and enums.
 * @version 1.0.0
 * @date 2025-03-24
 *
 * Copyright (c) 2025 The n-hop technologies Limited. All Rights Reserved.
 *
 */
#ifndef INCLUDE_C_BATS_TYPES_H_
#define INCLUDE_C_BATS_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bats_context_t* bats_context_handle_t;
typedef struct bats_protocol_t* bats_protocol_handle_t;
typedef struct bats_config_t* bats_config_handle_t;
typedef struct bats_connection_t* bats_connection_handle_t;

typedef enum {
  bats_log_level_trace = 0,
  bats_log_level_debug,
  bats_log_level_info,
  bats_log_level_warn,
  bats_log_level_error,
} bats_log_level_t;

typedef enum {
  bats_error_none = 0,
  bats_error_invalid_param,
  bats_error_listen_failed,
  bats_error_connect_failed,
  bats_error_accept_failed,
  bats_error_memory,
  bats_error_timeout,
  bats_error_connection_closed,
  bats_error_send_failed,
} bats_error_t;

typedef enum {
  bats_transport_btp = 0,      // Not implemented.
  bats_transport_brtp,         // Not implemented.
  bats_transport_brctp,        // Not implemented.
  bats_transport_transparent,  // udp, no fec, no cc.
} bats_transport_mode_t;

typedef enum {
  bats_cc_none = 0,   // No congestion control.
  bats_cc_bbr = 1,    // BBR congestion control.
  bats_cc_bbrv2 = 2,  // Not implemented.
  bats_cc_bbrv3 = 3,  // Not implemented.
  bats_cc_gcc,        // Not implemented.
} bats_congestion_control_t;

typedef enum {
  bats_frame_transparent = 0,  // no bats header
  bats_frame_header_min = 3,   // only has compatible header
  bats_frame_header_v1 = 2,    // has compatible header and coding header
  bats_frame_header_v0 = 1,    // `bats_framework_header`
} bats_frame_type_t;

/// @brief IOContext will emit those events when the state of the connection changes. Within one BatsConnection,the
/// callback is thread-safe.
typedef enum {
  bats_connection_none = 0,
  bats_connection_failed,             // conenction failed.
  bats_connection_established,        // connection established.
  bats_connection_timeout,            // connection timeout in 2s.
  bats_connection_shutdown_by_peer,   // connection shutdown by peer.
  bats_connection_writable,           // connection writable, ready to send data.
  bats_connection_data_received,      // connections has received data from peer.
  bats_connection_send_complete,      // connection sent last data complete.
  bats_connection_buffer_full,        // unable to write since the buffer of this connection is full.
  bats_connection_closed,             // connection closed.
  bats_connection_error,              // some errors in current connection
  bats_connection_already_connected,  // connection already established.
} bats_conn_event_t;

typedef enum {
  bats_listen_none = 0,
  bats_listen_new_connection,
  bats_listen_failed,
  bats_listen_success,  // listen success.
  bats_listen_accepted_error,
  bats_listen_already_in_listen,  // already in listen state.
} bats_listen_event_t;

typedef void (*bats_connection_callback_t)(bats_connection_handle_t conn, bats_conn_event_t event,
                                           const unsigned char* data, int length, void* user_data);

typedef void (*bats_listen_callback_t)(bats_connection_handle_t conn, bats_listen_event_t event, void* user_data);

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_C_BATS_TYPES_H_
