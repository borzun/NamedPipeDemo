#pragma once

#include <windows.h>
#include <memory>
#include <mutex>

// Separate struct to handle the pipe
struct PipeInstance {
  PipeInstance(size_t id, HANDLE pipe);

  size_t client_id;
  HANDLE pipe_handle;

  // TODO - need to double check whether we really need this mutex when Server /
  // Thread is destroyed
  std::mutex mutex;

  // Closing the pipe
  bool Close();
};