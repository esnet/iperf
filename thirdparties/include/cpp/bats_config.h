/**
 * @file bats_config.h
 * @author Lei Peng (peng.lei@n-hop.com)
 * @brief
 * @version 1.0.0
 * @date 2025-01-01
 *
 * Copyright (c) 2025 The n-hop technologies Limited. All Rights Reserved.
 *
 */
#ifndef INCLUDE_CPP_BATS_CONFIG_H_
#define INCLUDE_CPP_BATS_CONFIG_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using octet = unsigned char;
using octetVec = std::vector<octet>;
using octVecIter = std::vector<octet>::iterator;
using octVecConstIter = std::vector<octet>::const_iterator;

enum class TransMode : uint8_t {
  BTP = 0,          // Not implemented.
  BRTP = 1,         // Not implemented.
  BRCTP = 2,        // Not implemented.
  TRANSPARENT = 3,  // udp, no fec, no cc.
};

enum class CongestionControl : uint8_t {
  None = 0,   // No congestion control.
  BBR = 1,    // BBR congestion control.
  BBRv2 = 3,  // Not implemented.
  BBRv3 = 4,  // Not implemented.
  GCC = 5,    // Not implemented.
};

enum FrameType : uint8_t {
  TRANSPARENT = 0,      // no bats header
  BATS_HEADER_MIN = 3,  // only has compatible header
  BATS_HEADER_V1 = 2,   // has compatible header and coding header
  BATS_HEADER_V0 = 1,   // `bats_framework_header`
};

enum class BatsListenEvent : uint8_t {
  BATS_LISTEN_NONE = 0,
  BATS_LISTEN_NEW_CONNECTION,     // new connection is accepted.
  BATS_LISTEN_FAILED,             // failed to do listen.
  BATS_LISTEN_SUCCESS,            // listen success.
  BATS_LISTEN_ACCEPTED_ERROR,     // accepted connection error.
  BATS_LISTEN_ALREADY_IN_LISTEN,  // already in listen state.
};

/// @brief IOContext will emit those events when the state of the connection changes. Within one BatsConnection,the
/// callback is thread-safe.
enum class BatsConnEvent : uint8_t {
  BATS_CONNECTION_NONE = 0,
  BATS_CONNECTION_FAILED,             // conenction failed.
  BATS_CONNECTION_ESTABLISHED,        // connection established.
  BATS_CONNECTION_TIMEOUT,            // connection timeout in 2s.
  BATS_CONNECTION_SHUTDOWN_BY_PEER,   // connection shutdown by peer.
  BATS_CONNECTION_WRITABLE,           // connection writable, ready to send data.
  BATS_CONNECTION_DATA_RECEIVED,      // connections has received data from peer.
  BATS_CONNECTION_SEND_COMPLETE,      // connection sent last data complete.
  BATS_CONNECTION_BUFFER_FULL,        // unable to write since the buffer of this connection is full.
  BATS_CONNECTION_CLOSED,             // connection closed.
  BATS_CONNECTION_ERROR,              // some errors in current connection
  BATS_CONNECTION_ALREADY_CONNECTED,  // connection already established.
};

class BatsConfigImpl;
/// @brief The configuration for BATS protocol instance.
class BatsConfiguration {
 public:
  BatsConfiguration();
  ~BatsConfiguration();
  /// @brief Set the transport mode. Currently, only UDP and TCP are supported.
  /// @param mode
  void SetMode(TransMode mode);
  /// @brief Set the congestion control algorithm. In BATS protocol, transmission control and the congestion control are
  /// decoupled.
  /// @param cc Congestion control algorithm. currently, only BBR is supported.
  void SetCC(CongestionControl cc);
  /// @brief Set the timeout for connecting to the remote address.
  void SetTimeout(int timeout);
  /// @brief Set the TCP/UDP socket port which is used for listening or connecting.
  /// @param local_port The local port to be set.
  void SetLocalPort(int local_port);
  void SetLocalAddress(const std::string& local_addr);
  /// @brief The certificate file and key file are used for TLS connection.
  /// @param cert_file
  void SetCertFile(const std::string& cert_file);
  void SetKeyFile(const std::string& key_file);
  /// @brief Enable the compression for the data transmission.
  void EnableCompression(bool enable);
  void EnableEncryption(bool enable);
  /// @brief Choose the protocol frame type.
  /// @param frame_type
  void SetFrameType(FrameType frame_type);
  /// @brief Set the remote address and port which is used for connecting.
  /// @param remote_addr The remote address to be set, in IPv4/IPv6 string format.
  void SetRemoteAddr(const std::string& remote_addr);
  /// @brief set the remote port which is used for connecting.
  /// @param remote_port The remote port to be set.
  void SetRemotePort(int remote_port);
  const BatsConfigImpl& GetImpl() const { return *impl_; }
  friend std::ostream& operator<<(std::ostream& os, const BatsConfiguration& config);

 private:
  BatsConfigImpl* impl_ = nullptr;
};

#endif  // INCLUDE_CPP_BATS_CONFIG_H_
