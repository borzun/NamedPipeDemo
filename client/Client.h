#pragma once

#include <memory>
#include <string>
#include "Types.h"

class IDataSource;
class ClientRequest;
class Pipe;
class ResponseParser;

// Starting point of Client application.
// When creating this class, you can specify next parameters:
//	1. name of the pipe which should be created
//	2. data_source, from which client will read data requests to server;
//	3. parser - Parser of server responses on client's requests
//	4. exec_policy - Sync or Async calls to server.
//
// After you created a server, you can call a blocking Start() method, which
// will send data to the pipe until there will be data in data_source.
class Client {
 public:
  Client(const std::string& pipe_name, std::shared_ptr<IDataSource> data_source,
         std::shared_ptr<ResponseParser> parser, ExecutionPolicy exec_policy);

  bool Start();

 private:
  bool ConnectToPipe();
  bool ExecuteRequestSync(RequestId request_id, const RawDataType& data_to_send,
                          const ClientRequest& request);
  bool ExecuteRequestAsync(RequestId request_id, const RawDataType& data_to_send,
                           const ClientRequest& request);

 private:
  RequestId request_id_counter_ = 0;
  ExecutionPolicy exec_policy_;
  std::shared_ptr<IDataSource> data_source_;
  std::shared_ptr<ResponseParser> parser_;
  std::shared_ptr<Pipe> pipe_;
};