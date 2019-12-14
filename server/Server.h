#pragma once

#include <windows.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include "PipeInstance.h"
#include "ServerResponse.h"
#include "Types.h"

// This class is the starting point of the NamedPipeDemo::server module.
// It will create a named pipe and wait till clients will connect to that pipe.
// When client connects to the pipe, the server will automatically starts a new
// thread to process requests from that client. So, basically the server is more
// or less the same as this one -
// https://docs.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
class Server {
 public:
  explicit Server(const std::string& pipe_name);
  ~Server();

  // This blocks the calling thread.
  // Starts the server by connecting to the pipe.
  bool Start();

 private:
  // non-movable, non-copyable
  Server(const Server& other) = delete;
  Server& operator=(const Server& other) = delete;

  void HandleClientConnection(size_t client_id, HANDLE pipe_handle);
  ServerResponse ParseClientRequest(size_t client_id, HANDLE pipe_handle, const RawDataType& data);
  bool SendResponseToClient(size_t client_id, HANDLE pipe_handle, ServerResponse& response);

 private:
  const std::string pipe_name_;

  size_t client_ids_counter_ = 0;
  std::atomic_bool is_closed_ = false;

  // Pipes data can be accessed from multiple threads, so, synchronization
  // required
  mutable std::mutex pipes_mutex_;
  std::unordered_map<size_t, std::shared_ptr<PipeInstance>> pipes_;

  // Container of threads, which are processing active clients:
  std::vector<std::thread> threads_;
};