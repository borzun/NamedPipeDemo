#include "Pipe.h"

#include <conio.h>
#include <stdio.h>
#include <tchar.h>

#include <sstream>
#include "Logger.h"

static constexpr auto kLogTag = "Pipe";
static constexpr auto kBufSize = 4096;

namespace {

struct WriteResponseData {
  ~WriteResponseData() {
    delete overlapped;
    if (completion_event != INVALID_HANDLE_VALUE) {
      CloseHandle(completion_event);
    }

    if (wait_handle != INVALID_HANDLE_VALUE) {
      UnregisterWait(wait_handle);
    }
  }
  RequestId request_id;
  bool wait_for_response;
  Pipe::WriteAsyncResponseCallback callback;
  LPOVERLAPPED overlapped;
  HANDLE completion_event = INVALID_HANDLE_VALUE;
  HANDLE pipe_handle = INVALID_HANDLE_VALUE;
  HANDLE wait_handle = INVALID_HANDLE_VALUE;
};

// callback to handle the async write request to server
void CALLBACK HandleAsyncResponseOnWriteToServer(_In_ PVOID lpParameter,
                                                 _In_ BOOLEAN TimerOrWaitFired) {
#ifndef NDEBUG
  Logger::LogDebug("Received Async write response from server...");
#endif
  auto* data = reinterpret_cast<WriteResponseData*>(lpParameter);
  if (!data) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << " ERROR - invalid WriteResponseData!"));
    return;
  }

  if (auto callback = data->callback) {
    DWORD bytes_written = 0;
    if (!GetOverlappedResult(data->pipe_handle, data->overlapped, &bytes_written, true)) {
      Logger::LogError(Logger::to_string(
          std::stringstream() << kLogTag << ": ERROR - failed to get overlapped results!"));
    } else {
      callback(data->request_id, data->wait_for_response);
    }
  }

  delete data;
}

struct ReadResponseData {
  ~ReadResponseData() {
    delete overlapped;
    if (completion_event != INVALID_HANDLE_VALUE) {
      CloseHandle(completion_event);
    }
    if (wait_handle != INVALID_HANDLE_VALUE) {
      UnregisterWait(wait_handle);
    }
  }
  std::shared_ptr<RawDataType> data;
  Pipe::ReadAsyncResponseCallback callback;
  LPOVERLAPPED overlapped;
  HANDLE completion_event;
  HANDLE pipe_handle = INVALID_HANDLE_VALUE;
  HANDLE wait_handle = INVALID_HANDLE_VALUE;
};

// callback to handle asycn response from server
void CALLBACK HandleAsyncReadResponseFromServer(_In_ PVOID lpParameter,
                                                _In_ BOOLEAN TimerOrWaitFired) {
#ifndef NDEBUG
  Logger::LogDebug("Received Sync read response from server...");
#endif
  auto* response_data = reinterpret_cast<ReadResponseData*>(lpParameter);
  if (!response_data) {
    Logger::LogError(Logger::to_string(
        std::stringstream() << kLogTag << " ERROR - received invalid response from server!"));
    return;
  }

  if (auto callback = response_data->callback) {
    DWORD bytes_written = 0;
    if (!GetOverlappedResult(response_data->pipe_handle, response_data->overlapped, &bytes_written,
                             true)) {
      Logger::LogError(Logger::to_string(
          std::stringstream() << kLogTag << ": ERROR - failed to get overlapped results!"));
    } else {
      auto data = response_data->data;
      data->resize(bytes_written);

      callback(*data.get());
    }
  }

  delete response_data;
}
}  // namespace

Pipe::Pipe(const std::string& name, ExecutionPolicy exec_policy)
    : pipe_name_(name), exec_policy_(exec_policy) {}

Pipe::~Pipe() { DisconnectFromServer(); }

bool Pipe::Connect(std::chrono::seconds wait_timeout) {
  if (IsConnected()) {
    return true;
  }

  // TODO - is it correct conversion?
  LPTSTR lpsz_pipe_name = (LPTSTR)pipe_name_.c_str();

  while (true) {
    // TODO: what about FILE_FLAG_WRITE_THROUGH???
    auto file_attribute = exec_policy_ == ExecutionPolicy::Async ? FILE_FLAG_OVERLAPPED : 0;

    auto handle = CreateFile(lpsz_pipe_name,                // pipe name
                             GENERIC_READ | GENERIC_WRITE,  // read and write access
                             0,                             // no sharing
                             NULL,                          // default security attributes
                             OPEN_EXISTING,                 // opens existing pipe
                             file_attribute,                // default attributes
                             NULL);                         // no template file

    if (handle != INVALID_HANDLE_VALUE) {
      pipe_handle_ = handle;
      const std::string exec_policy_str =
          exec_policy_ == ExecutionPolicy::Async ? "async" : "sync";
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag << ": created a client pipe=" << handle
                                         << " with " << exec_policy_str << " policy!"));
      return true;
    }

    // From docs:
    // https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-client?redirectedfrom=MSDN
    // In case all pipes are busy, need to wait till pipe will be available
    if (GetLastError() != ERROR_PIPE_BUSY) {
      Logger::LogError(Logger::to_string(
          std::stringstream() << kLogTag << ": could not open pipe, error=" << GetLastError()));
      return false;
    }

    // wait till instance of pipe will be ready
    const auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(wait_timeout);
    if (!WaitNamedPipe(lpsz_pipe_name, static_cast<DWORD>(milis.count()))) {
      Logger::LogError(Logger::to_string(
          std::stringstream() << kLogTag << ": wait timeout, error=" << GetLastError()));
      return false;
    }
  }

  return true;
}

bool Pipe::DisconnectFromServer() {
  if (IsConnected()) {
    BOOL ret = CloseHandle(pipe_handle_);
    if (!ret) {
      Logger::LogError(Logger::to_string(std::stringstream()
                                         << kLogTag << ": error during closing the pipe="
                                         << pipe_handle_ << ", error=" << GetLastError()));
    } else {
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << "Successfully closed the pipe=" << pipe_handle_));
    }

    pipe_handle_ = nullptr;
    return ret;
  }

  return false;
}

bool Pipe::IsConnected() const { return pipe_handle_ != nullptr; }

bool Pipe::SendDataToServerSync(const RawDataType& data) {
  constexpr auto kCharSize = sizeof(RawDataType::value_type);
  DWORD bytes_to_write = data.size() * kCharSize;

  DWORD bytes_written = 0;
  bool success = true;

  success = WriteFile(pipe_handle_, data.data(), bytes_to_write, &bytes_written,
                      nullptr);  // sync
  if (!success && GetLastError() != ERROR_IO_PENDING) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": failed to write to pipe, pipe="
                                       << pipe_handle_ << ", error=" << GetLastError()));
    return false;
  } else {
    Logger::LogDebug(Logger::to_string(std::stringstream()
                                       << kLogTag << ": send data to pipe=" << pipe_handle_
                                       << ", bytes=" << bytes_written));
  }

  return true;
}

// Block the thread and wait for the response from pipe (via ReadData)
std::pair<bool, RawDataType> Pipe::ReadDataFromServerSync() {
  RawDataType res;

  RawDataType tmp(kBufSize);
  BOOL success = false;
  do {
    // Read the server's response from the pipe.
    DWORD bytes_read;
    success = ReadFile(pipe_handle_, tmp.data(), tmp.size() * sizeof(RawDataType::value_type),
                       &bytes_read,
                       nullptr);  // not overlapped  - sync

    if (!success && GetLastError() != ERROR_MORE_DATA) {
      // error happened
      break;
    }

    res.insert(res.end(), tmp.begin(), std::next(tmp.begin(), bytes_read));

  } while (!success);  // repeat loop if ERROR_MORE_DATA

  if (!success) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": ERROR - failed to read from pipe, error="
                                       << GetLastError()));
    return std::make_pair(false, RawDataType{});
  }

  return std::make_pair(true, res);
}

bool Pipe::SendDataToServerAsync(const RawDataType& data, WriteAsyncResponseCallback callback,
                                 RequestId request_id, bool wait_for_response) {
  constexpr auto kCharSize = sizeof(RawDataType::value_type);
  DWORD bytes_to_write = data.size() * kCharSize;

  DWORD bytes_written = 0;
  bool success = true;

  LPOVERLAPPED overlapped = new OVERLAPPED;
  auto completion_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  overlapped->hEvent = completion_event;
  overlapped->Offset = 0;
  overlapped->OffsetHigh = 0;
  overlapped->Internal = 0;
  overlapped->InternalHigh = 0;

  success = WriteFile(pipe_handle_, data.data(), bytes_to_write, &bytes_written, overlapped);
  // If the overlapped operation on pipe is still in progress (ERROR_IO_PENDING), no need to return
  // Just schedule the call and wait when the CompletionEvent object will be signalled
  if (!success && GetLastError() != ERROR_IO_PENDING) {
    delete overlapped;
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": failed to write to pipe, pipe="
                                       << pipe_handle_ << ", error=" << GetLastError()));
    return false;
  } else {
#ifndef NDEBUG
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": async send data to pipe=" << pipe_handle_));
#endif
  }

  // register callback
  WriteResponseData* response = new WriteResponseData();
  response->overlapped = overlapped;
  response->pipe_handle = pipe_handle_;
  response->callback = callback;
  response->request_id = request_id;
  response->wait_for_response = wait_for_response;
  response->completion_event = completion_event;

  if (!RegisterWaitForSingleObject(
          &response->wait_handle, completion_event, &HandleAsyncResponseOnWriteToServer,
          reinterpret_cast<void*>(response), INFINITE, WT_EXECUTEONLYONCE)) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag
                                       << ": ERROR - Can't register waiting on "
                                          "completion event on WriteData!"));
    delete response;
  }
  return true;
}

bool Pipe::ReadDataFromServerAsync(ReadAsyncResponseCallback callback) {
  RawDataType res;

  BOOL success = false;
  {
    // Read the server's response from the pipe.
    DWORD bytes_read;

    LPOVERLAPPED overlapped = new OVERLAPPED;
    auto data = std::make_shared<RawDataType>(kBufSize);
    auto completion_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    overlapped->hEvent = completion_event;
    overlapped->Offset = 0;
    overlapped->OffsetHigh = 0;
    overlapped->Internal = 0;
    overlapped->InternalHigh = 0;

    success = ReadFile(pipe_handle_, data->data(), data->size() * sizeof(RawDataType::value_type),
                       &bytes_read,
                       overlapped);  // overlapped - async

	// If the overlapped operation on pipe is still in progress (ERROR_IO_PENDING), no need to return
	// Just schedule the call and wait when the CompletionEvent object will be signalled
    if (!success && GetLastError() != ERROR_IO_PENDING) {
      delete overlapped;
      Logger::LogError(Logger::to_string(std::stringstream()
                                         << kLogTag << ": ERROR - failed to read from pipe, error="
                                         << GetLastError()));
      return false;
	}
	else {
#ifndef NDEBUG
		Logger::LogDebug(Logger::to_string(
			std::stringstream() << kLogTag << ": async read data to pipe=" << pipe_handle_ << " get_last_error=" << GetLastError()));
#endif
	}

    ReadResponseData* response = new ReadResponseData();
    response->overlapped = overlapped;
    response->data = data;
    response->pipe_handle = pipe_handle_;
    response->callback = callback;
    response->completion_event = completion_event;

    if (!RegisterWaitForSingleObject(
            &response->wait_handle, completion_event, &HandleAsyncReadResponseFromServer,
            reinterpret_cast<void*>(response), INFINITE, WT_EXECUTEONLYONCE)) {
      Logger::LogError(Logger::to_string(std::stringstream()
                                         << kLogTag
                                         << ": ERROR - Can't register waiting on "
                                            "completion event on ReadData!"));
      delete response;
    }
    return true;
  }
}