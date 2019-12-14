#include "Client.h"

#include <sstream>
#include <thread>
#include "ClientRequest.h"
#include "DataSerializer.h"
#include "IDataSource.h"
#include "Logger.h"
#include "Pipe.h"
#include "ResponseParser.h"

static constexpr auto kLogTag = "Client";

static RawDataType CreateRequestIdData(RequestId request) {
  std::stringstream ss;
  ss << "#r";
  DataSerializer::Serialize<int>(ss, request);

  return DataSerializer::ConvertToRawData(ss.str());
}

Client::Client(const std::string& pipe_name, std::shared_ptr<IDataSource> data_source,
               std::shared_ptr<ResponseParser> parser, ExecutionPolicy exec_policy)
    : exec_policy_(exec_policy),
      data_source_(std::move(data_source)),
      parser_(std::move(parser)),
      pipe_(std::make_shared<Pipe>(pipe_name, exec_policy)) {}

bool Client::Start() {
  if (!data_source_ || !parser_) {
    Logger::LogError(
        Logger::to_string(std::stringstream() << kLogTag << ": ERROR - invalid arguments!"));
    return false;
  }

  if (!ConnectToPipe()) {
    Logger::LogError("ERROR - failed to connect - terminating a client!");
    return false;
  }

  // keep getting while there are some data in a stream
  while (data_source_->IsGood()) {
    auto request = data_source_->ReadRequest();
    auto data = request.GetData();
    if (data.empty()) {
      // no data to send - retry!
      continue;
    }

    auto request_id = ++request_id_counter_;
    parser_->RegisterRequest(request_id, request);

    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": sending request=" << request_id
                            << ", data=" << DataSerializer::ConvertRawDataToString(data)));

    // adding the request id data to the sending data:
    auto data_to_send = CreateRequestIdData(request_id);
    data_to_send.insert(data_to_send.end(), data.begin(), data.end());

    bool result = true;
    // Processing request - either sync or async:
    if (exec_policy_ == ExecutionPolicy::Sync) {
      if (!ExecuteRequestSync(request_id, data_to_send, request)) {
        result = false;
      }
    } else {
      if (!ExecuteRequestAsync(request_id, data_to_send, request)) {
        result = false;
      }
    }

    if (result == false) {
      // if request fails and the pipe is being closed -
      // try to connect to server
      if (GetLastError() == ERROR_NO_DATA) {
        Logger::LogDebug("Trying to reconnect to server...");
        pipe_->DisconnectFromServer();
        if (!ConnectToPipe()) {
          Logger::LogError("ERROR - failed to connect - terminating a client!");
          return false;
        }
      } else {
        Logger::LogError("ERROR - terminating client!");
        return false;
      }
    }
  }
  return true;
}

bool Client::ExecuteRequestSync(RequestId request_id, const RawDataType& data_to_send,
                                const ClientRequest& request) {
  if (!pipe_->SendDataToServerSync(data_to_send)) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": ERROR - failed to sync send a  request="
                                       << request_id));
    // TODO: need to provide better fallback strategy...
    return false;
  }

  if (request.NeedToWaitForResponse()) {
    auto data = pipe_->ReadDataFromServerSync();
    if (!data.first) {
      Logger::LogError(Logger::to_string(std::stringstream() << "ERROR: Failed to read pipe!"));
    } else {
      parser_->ParseResponse(data.second);
    }
  }

  return true;
}

bool Client::ExecuteRequestAsync(RequestId request_id, const RawDataType& data_to_send,
                                 const ClientRequest& request) {
  auto weak_parser = std::weak_ptr<ResponseParser>(parser_);
  auto handle_read_response = [weak_parser](RawDataType data) {
    // Response can be received after Client is destroyed - need to handle that:
    auto parser = weak_parser.lock();
    if (!parser) {
      return;
    }
    parser->ParseResponse(std::move(data));
  };

  auto weak_pipe = std::weak_ptr<Pipe>(pipe_);
  auto handle_write_response = [handle_read_response, weak_pipe](RequestId request_id,
                                                                 bool read_data) {
    auto pipe = weak_pipe.lock();
    // Response can be received after Client is destroyed:
    if (!pipe) {
      return;
    }
#ifndef NDEBUG
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag
                            << ": Received async reponse on write data to server, request_id="
                            << request_id << "; wait_for_response=" << read_data));
#endif
    if (read_data) {
      pipe->ReadDataFromServerAsync(handle_read_response);
    }
  };

  if (!pipe_->SendDataToServerAsync(data_to_send, handle_write_response, request_id,
                                    request.NeedToWaitForResponse())) {
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": ERROR - failed to async send a request="
                                       << request_id << "!"));
    return false;
  }

  return true;
}

bool Client::ConnectToPipe() {
  size_t retry_iteration = 0;
  constexpr auto wait_timeout = 20000;  // seconds
  while (!pipe_->Connect()) {
    if (++retry_iteration == 100) {
      Logger::LogError(Logger::to_string(
          std::stringstream() << kLogTag
                              << ": ERROR - TIMEOUT, failed to connect to pipe. Exiting...!"));
      return false;
    }
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag
                                       << ": ERROR - NO INSTANCE of pipe is "
                                          "created. Will retry in 5 seconds!"));
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  return true;
}
