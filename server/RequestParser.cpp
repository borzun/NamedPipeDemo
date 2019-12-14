#include "RequestParser.h"

#include <sstream>
#include "CustomClassParser.h"
#include "DataDeserializer.h"
#include "Logger.h"

constexpr auto kLogTag = "RequestParser";

RequestParser::RequestParser(size_t client_id) : client_id_(client_id) {}

bool RequestParser::ParseRequest(const RawDataType& request, ServerResponse& response) const {
  size_t idx = 0;
  response = ServerResponse{};

  // Try to parse a request id
  auto request_id = ParseRequestId(request, idx);

  const auto tmp_idx = idx;  // just for debug purposes
  if (auto [success, value] = RegularTypeParaser::Parse<int>(request, idx); success) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id
                            << "] Received int value from client: " << value));
    return true;
  } else if (auto [success, value] = RegularTypeParaser::Parse<double>(request, idx); success) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id
                            << "] Received double value from client: " << value));
    return true;
  } else if (auto [success, value] = RegularTypeParaser::Parse<std::string>(request, idx);
             success) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id
                            << "] Received std::string value from client: " << value.c_str()));
    return true;
  } else if (auto [success, resp] = CustomClassParser(client_id_, request_id).Parse(request, idx);
             success) {
    auto req_str = std::string(std::next(request.begin(), tmp_idx), request.end());
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id
                            << "] Processed CustomClass request=" << req_str));
    response = std::move(resp);
    response.SetRequestId(request_id);
    return true;
  }

  Logger::LogError(Logger::to_string(std::stringstream() << kLogTag << ": [client=" << client_id_
                                                         << ", request=" << request_id
                                                         << "] ERROR - can't parse data!"));
  return false;
}

RequestId RequestParser::ParseRequestId(const RawDataType& request, size_t& seek_idx) const {
  if (request.size() < seek_idx + 2) {
    return -1;
  }

  size_t tmp_idx = seek_idx;
  if (request[tmp_idx++] == '#' && request[tmp_idx++] == 'r') {
    if (auto [success, value] = RegularTypeParaser::Parse<RequestId>(request, tmp_idx); success) {
      seek_idx = tmp_idx;
      return value;
    }
  }

  return -1;
}