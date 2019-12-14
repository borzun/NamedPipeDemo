#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <utility>

#include <windows.h>
#include "Types.h"

class Pipe final {
 public:
  using WriteAsyncResponseCallback = std::function<void(RequestId, bool)>;
  using ReadAsyncResponseCallback = std::function<void(RawDataType)>;

 public:
  Pipe(const std::string& name, ExecutionPolicy exec_policy);
  ~Pipe();

  // Connects to the pipe.
  // In case all pipe instances are busy, will wait for specific amount of time
  // (default - 20 secs) In case pipe is already connected, will return true.
  bool Connect(std::chrono::seconds wait_timeout = std::chrono::seconds(20));

  // Disconect this pipe instance.
  bool DisconnectFromServer();

  // Checks whether Pipe is already connected.
  bool IsConnected() const;

  // ---- Sync exectuion:
  // Writes data to a client's instance of a pipe.
  bool SendDataToServerSync(const RawDataType& data);
  // Block the thread and wait for the response from pipe (via ReadData)
  std::pair<bool, RawDataType> ReadDataFromServerSync();

  // ----- Async execution:
  bool SendDataToServerAsync(const RawDataType& data,
                             WriteAsyncResponseCallback callback,
                             RequestId request_id, bool wait_for_response);
  bool ReadDataFromServerAsync(ReadAsyncResponseCallback callback);

 private:
  std::string pipe_name_;
  const ExecutionPolicy exec_policy_;

  HANDLE pipe_handle_ = nullptr;
};