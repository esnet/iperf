/**
 * @file c_bats_cpi.h
 * @author Lei Peng (peng.lei@n-hop.com)
 * @brief  BATS protocol c-style interfaces.
 * @version 1.0.0
 * @date 2025-03-24
 *
 * Copyright (c) 2025 The n-hop technologies Limited. All Rights Reserved.
 *
 */

#ifndef INCLUDE_C_BATS_API_H_
#define INCLUDE_C_BATS_API_H_

#include "c_bats_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ================= Context ====================
bats_context_handle_t bats_context_create();
void bats_context_destroy(bats_context_handle_t ctx);
void bats_context_set_log_level(bats_context_handle_t ctx, bats_log_level_t level);

// ================= Config =====================
bats_config_handle_t bats_config_create();
void bats_config_destroy(bats_config_handle_t config);
void bats_config_set_mode(bats_config_handle_t config, bats_transport_mode_t mode);
void bats_config_set_timeout(bats_config_handle_t config, int timeout);
void bats_config_set_local_addr(bats_config_handle_t config, const char* local_addr);
void bats_config_set_local_port(bats_config_handle_t config, int local_port);
void bats_config_set_remote_addr(bats_config_handle_t config, const char* remote_addr);
void bats_config_set_remote_port(bats_config_handle_t config, int remote_port);
void bats_config_set_cc(bats_config_handle_t config, bats_congestion_control_t cc);
void bats_config_cert_file(bats_config_handle_t config, const char* cert_file);
void bats_config_key_file(bats_config_handle_t config, const char* key_file);
void bats_config_enable_compression(bats_config_handle_t config, int enable);
void bats_config_enable_encryption(bats_config_handle_t config, int enable);
void bats_config_set_frame_type(bats_config_handle_t config, bats_frame_type_t frame_type);

// ================= Protocol instance =====================
bats_protocol_handle_t bats_protocol_create(bats_context_handle_t ctx, bats_config_handle_t config);
void bats_protocol_destroy(bats_protocol_handle_t protocol);
bats_error_t bats_protocol_start_connect(bats_protocol_handle_t protocol, const char* remote_addr, uint16_t remote_port,
                                         bats_connection_callback_t c_callback, void* user_data);
bats_error_t bats_protocol_start_listen(bats_protocol_handle_t protocol, const char* local_addr, uint16_t local_port,
                                        bats_listen_callback_t c_callback, void* user_data);

// ================= Protocol Connection =====================
bats_error_t bats_connection_send_data(bats_connection_handle_t conn, const unsigned char* data, int length);
bats_error_t bats_connection_send_file(bats_connection_handle_t conn, const char* file_name);
void bats_connection_set_callback(bats_connection_handle_t conn, bats_connection_callback_t c_callback,
                                  void* user_data);
int bats_connection_recv_data(bats_connection_handle_t conn, unsigned char* data, int max_length);
int bats_connection_is_writable(bats_connection_handle_t conn);

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_C_BATS_API_H_
