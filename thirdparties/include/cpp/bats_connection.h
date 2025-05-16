/**
 * @file bats_connection.h
 * @author Lei Peng (peng.lei@n-hop.com)
 * @brief
 * @version 1.0.0
 * @date 2025-03-19
 *
 * Copyright (c) 2025 The n-hop technologies Limited. All Rights Reserved.
 *
 */
#ifndef INCLUDE_CPP_BATS_CONNECTION_H_
#define INCLUDE_CPP_BATS_CONNECTION_H_

#include <functional>
#include <memory>
#include <string>

#include "bats_config.h"

class IBatsConnection;
using IBatsConnPtr = std::shared_ptr<IBatsConnection>;
using ConnectionCallback = std::function<void(const IBatsConnPtr&, const BatsConnEvent&, const octet*, int, void*)>;
using ListenCallback = std::function<void(const IBatsConnPtr& connection, const BatsListenEvent&, void*)>;

/// @brief BatsConnection is the interface for writing/reading data to/from the network.
class IBatsConnection {
 public:
  virtual ~IBatsConnection() = default;
  /// @brief Set the callback function to receive the connection events(read/write/close).
  /// @param cb The callback function. Don't block in the callback function.
  virtual void SetConnectionCallback(const ConnectionCallback& cb) = 0;
  virtual void SetConnectionCallback(ConnectionCallback&& cb) = 0;
  /// @brief Interface for sending data to the network.
  /// @param data The data to be sent.
  /// @return true if the data is sent successfully, false otherwise. Sending failure can be caused by the following
  /// reasons:
  ///         1. The connection is closed.
  ///         2. The connection is not writable (user buffer is full).
  ///         3. The connection is not ready (not connected/established).
  virtual bool SendData(const octetVec& data) = 0;
  virtual bool SendData(const octet* data, int length) = 0;
  /// @brief Send a file to the network. only reliable connection(BRTP, BRCTP) supports this function.
  /// @param file_name The file with full path to be sent.
  /// @return
  virtual bool SendFile(const std::string& file_name) = 0;
  /// @brief Receive data from the network. This function is not implemented in the current version. Recommend receiving
  /// data in connection callback.
  /// @param data The buffer to store the received data.
  /// @param length The available length of the buffer.
  /// @return received bytes if success, -1 if failed, 0 means connection closed.
  virtual int RecvData(octet* data, int length) = 0;
  /// @brief Indicate whether the connection is writable at current time.
  /// @return true if the connection is writable, false otherwise.
  virtual bool IsWritable() = 0;
};

#endif  // INCLUDE_CPP_BATS_CONNECTION_H_
