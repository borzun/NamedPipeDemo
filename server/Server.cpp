#include "Server.h"

#include <sstream>
#include "DataSerializer.h"
#include "Logger.h"
#include "RequestParser.h"

static constexpr auto kLogTag = "Server";

static constexpr DWORD kBuffSize = 4096;

namespace {
RawDataType CreateRequestIdData(RequestId request) {
  std::stringstream ss;
  ss << "#r";
  DataSerializer::Serialize<int>(ss, request);

  return DataSerializer::ConvertToRawData(ss.str());
}
}  // namespace

Server::Server(const std::string &pipe_name) : pipe_name_(pipe_name) {}

Server::~Server() {
  is_closed_.store(true);

  std::unordered_map<size_t, std::shared_ptr<PipeInstance>> pipes;
  {
    std::lock_guard<std::mutex> locker(pipes_mutex_);
    pipes = std::move(pipes_);
  }
  // close all handles:
  for (auto p : pipes) {
    auto pipe_instance = p.second;

    // This is tricky one - we need to guarantee the thread-safe access to the
    // pipe instance
    std::lock_guard<std::mutex> locker(pipe_instance->mutex);
    pipe_instance->Close();
  }

  // Waiting for other threads:
  for (auto &thread : threads_) {
    thread.join();
  }
}

bool Server::Start() {
  // Same idea as in multi-threaded named pipe server
  // First, create a named pipe with read-write method
  // after that continuously waiting for new clients to a pipe
  while (true) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag
                            << ": waiting for client connection on NamedPipe: " << pipe_name_));
    LPTSTR lpsz_pipe_name = (LPTSTR)pipe_name_.c_str();
    HANDLE pipe_handle = INVALID_HANDLE_VALUE;
    pipe_handle =
        CreateNamedPipe(lpsz_pipe_name,
                        PIPE_ACCESS_DUPLEX,  // read/write access
                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,  // blocking mode
                        PIPE_UNLIMITED_INSTANCES,                               // max. instances
                        kBuffSize, kBuffSize,
                        0,         // client time-out
                        nullptr);  // default security attribute

    if (pipe_handle == INVALID_HANDLE_VALUE) {
      Logger::LogError(Logger::to_string(std::stringstream()
                                         << kLogTag << ": ERROR: failed to connect to pipe, error="
                                         << GetLastError()));
      return false;
    }

    // connect with a client
    BOOL connected = ConnectNamedPipe(pipe_handle, nullptr);
    if (connected || (GetLastError() == ERROR_PIPE_CONNECTED)) {
      auto client_id = client_ids_counter_++;
      {
        std::lock_guard<std::mutex> locker(pipes_mutex_);
        pipes_[client_id] = std::make_shared<PipeInstance>(client_id, pipe_handle);
      }

      Logger::LogDebug(Logger::to_string(
          std::stringstream() << kLogTag << ": connected a client=," << client_id
                              << " to a pipe=" << pipe_handle << ", stating a thread..."));
      std::thread thread(&Server::HandleClientConnection, this, client_id, pipe_handle);
      threads_.emplace_back(std::move(thread));
    } else {
      // The client could not connect, so close the pipe.
      CloseHandle(pipe_handle);
    }
  }

  return true;
}

void Server::HandleClientConnection(size_t client_id, HANDLE pipe_handle) {
  if (pipe_handle == nullptr) {
    Logger::LogError(
        Logger::to_string(std::stringstream() << kLogTag << ": ERROR - Invalid pipe handle!"));
    return;
  }

  Logger::LogError(Logger::to_string(
      std::stringstream() << kLogTag << ": Server started processing messages from client: "
                          << client_id << " using pipe handle: " << pipe_handle));

  std::shared_ptr<PipeInstance> pipe = nullptr;
  {
    std::lock_guard<std::mutex> locker(pipes_mutex_);
    pipe = pipes_[client_id];
  }
  // block and wait till server is up or client is alive
  while (!is_closed_) {
    // Here need to understand whether this is correct place to set a mutex
    // Overall, this is required in order to properly shutdown the Server.
    // Maybe it is not needed due to blocking behaviour of ReadFile and other
    // operations on this pipe.
    std::lock_guard<std::mutex> locker(pipe->mutex);

    std::vector<char> data(kBuffSize);
    DWORD bytes_read = 0;
    // read data from client
    BOOL success = ReadFile(pipe_handle, data.data(), data.size() * sizeof(char), &bytes_read,
                            nullptr);  // not overlapped I/O

    if (!success || bytes_read == 0) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        Logger::LogError(Logger::to_string(
            std::stringstream() << kLogTag
                                << ": the client is disconnected for client_id=" << client_id));
      } else {
        Logger::LogError(Logger::to_string(std::stringstream()
                                           << kLogTag << ": ERROR: ReadFile is failed, client_id="
                                           << client_id << ", error=" << GetLastError()));
      }

      pipe->Close();
      break;
    }

    data.resize(bytes_read);  // aka shrink to fit
    ServerResponse response = ParseClientRequest(client_id, pipe_handle, data);
    if (response.IsValid()) {
      if (!SendResponseToClient(client_id, pipe_handle, response)) {
        Logger::LogError(
            Logger::to_string(std::stringstream()
                              << kLogTag << ": ERROR: Failed to send back a response to client_id="
                              << client_id << " -  closing the client!"));
        return;
      }
    }
  }

  Logger::LogDebug(Logger::to_string(std::stringstream() << kLogTag << ": the thread for client="
                                                         << client_id << " is terminating..."));
}

ServerResponse Server::ParseClientRequest(size_t client_id, HANDLE pipe_handle,
                                          const RawDataType &data) {
  ServerResponse response;
  RequestParser(client_id).ParseRequest(data, response);
  return response;
}

bool Server::SendResponseToClient(size_t client_id, HANDLE pipe_handle, ServerResponse &response) {
  auto data = response.GetData();
  auto data_to_send = CreateRequestIdData(response.GetRequestId());
  data_to_send.insert(data_to_send.end(), data.begin(), data.end());

  auto bytes_to_write = data_to_send.size() * sizeof(char);

  DWORD bytes_written = 0;
  // Send back reply to a client:
  BOOL success = WriteFile(pipe_handle, data_to_send.data(), bytes_to_write, &bytes_written,
                           nullptr);  // not overlapped I/O

  if (!success || bytes_written != bytes_to_write) {
    auto error = GetLastError();
    response.HandleFailure(client_id, error);
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": ERROR: failed to write to client_id="
                                       << client_id << ", error=" << error));
    return false;
  } else {
    response.HandleSuccess(client_id);
  }
  return true;
}
