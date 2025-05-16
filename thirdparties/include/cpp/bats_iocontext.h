/**
 * @file bats_iocontext.h
 * @author Lei Peng (peng.lei@n-hop.com)
 * @brief
 * @version 1.0.0
 * @date 2025-03-19
 *
 * Copyright (c) 2025 The n-hop technologies Limited. All Rights Reserved.
 *
 */

#ifndef INCLUDE_CPP_BATS_IOCONTEXT_H_
#define INCLUDE_CPP_BATS_IOCONTEXT_H_

class IOContextImpl;
enum class BATSLogLevel : int {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
};
/// @brief IO event processing instance in BATS protocol; each instance will create at least one thread to process IO
/// events. IOContext can be shared by multiple protocol instances.
class IOContext {
 public:
  IOContext();
  virtual ~IOContext();
  IOContextImpl& GetImpl() const { return *impl_; }
  /// @brief Set the log level for current process. The log level can be set to TRACE, DEBUG, INFO, WARN, or ERROR.
  /// @param level
  void SetBATSLogLevel(BATSLogLevel level);

 protected:
  IOContextImpl* impl_ = nullptr;
};

#endif  // INCLUDE_CPP_BATS_IOCONTEXT_H_
