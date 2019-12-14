#include "PipeInstance.h"

#include <sstream>
#include <string>
#include "Logger.h"

static constexpr auto kLogTag = "PipeInstance";

PipeInstance::PipeInstance(size_t id, HANDLE pipe)
    : client_id(id), pipe_handle(pipe) {}

bool PipeInstance::Close() {
  if (!pipe_handle) {
    return false;
  }

  Logger::LogDebug(Logger::to_string(
      std::stringstream() << kLogTag << ": Closing pipe=" << pipe_handle
                          << " for client=" << client_id << "..."));

  if (!FlushFileBuffers(pipe_handle)) {
    Logger::LogError(Logger::to_string(
        std::stringstream()
        << kLogTag << ": ERROR - Failed to flush buffer, error="
        << GetLastError() << ", client_id=" << client_id));
    return false;
  }
  if (!DisconnectNamedPipe(pipe_handle)) {
    Logger::LogError(Logger::to_string(
        std::stringstream()
        << kLogTag << ": ERROR - Failed to disconnect pipe, error="
        << GetLastError() << ", client_id=" << client_id));
    return false;
  }
  CloseHandle(pipe_handle);

  Logger::LogDebug(Logger::to_string(
      std::stringstream() << kLogTag << ": Closed pipe=" << pipe_handle
                          << " for client=" << client_id << " successfully"));

  pipe_handle = nullptr;
  return true;
}