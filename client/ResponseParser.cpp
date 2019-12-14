#include "ResponseParser.h"

#include "ClassRepository.h"
#include "CustomClass.h"
#include "DataDeserializer.h"

#include <sstream>
#include "Logger.h"

static constexpr auto kLogTag = "ResponseParser";

bool ResponseParser::ParseResponse(const RawDataType& data) {
  size_t idx = 0;

  // Try to parse a request id from response
  auto request_id = ParseRequestId(data, idx);
  std::any any_value;

  if (ParseCustomClassResponse(data)) {
    return true;
  } else if (auto [success, value] = RegularTypeParaser::Parse<bool>(data, idx); success) {
#ifndef NDEBUG  // just for debug purposes
    Logger::LogDebug(Logger::to_string(std::stringstream()
                                       << "[request_id=" << request_id
                                       << "] Received bool value from server: " << value));
#endif
    any_value = value;
  } else if (auto [success, value] = RegularTypeParaser::Parse<int>(data, idx); success) {
#ifndef NDEBUG  // just for debug purposes
    Logger::LogDebug(Logger::to_string(std::stringstream()
                                       << "[request_id=" << request_id
                                       << "] Received int value from server: " << value));
#endif
    any_value = value;
  } else if (auto [success, value] = RegularTypeParaser::Parse<double>(data, idx); success) {
#ifndef NDEBUG  // just for debug purposes
    Logger::LogDebug(Logger::to_string(std::stringstream()
                                       << "[request_id=" << request_id
                                       << "] Received double value from server: " << value));
#endif
    any_value = value;
  } else if (auto [success, value] = RegularTypeParaser::Parse<std::string>(data, idx); success) {
#ifndef NDEBUG  // just for debug purposes
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << "[request_id=" << request_id
                            << "] Received std::string value from client: " << value.c_str()));
#endif
    any_value = value;
  } else {
    // TODO: handle error!
    Logger::LogError(Logger::to_string(std::stringstream()
                                       << kLogTag << ": Can't parse a request=" << request_id));
    return false;
  }

  // Notifying the ClientRequest about response
  {
    std::lock_guard<std::mutex> locker(requests_mutex_);

    decltype(requests_)::iterator iter = requests_.find(request_id);

    if (iter != requests_.end()) {
      iter->second.HandleSuccess(any_value);
      requests_.erase(iter);
    }
  }

  return true;
}

bool ResponseParser::ParseCustomClassResponse(const RawDataType& data) const {
  size_t idx = 0;
  if (data.empty() || data[idx++] != '#') {
    return false;
  }

  // Check that class name is the same as the CustomClass:
  if (auto [success, str] = RegularTypeParaser::Parse<std::string>(data, idx); success) {
    if (str != CustomClass::kClassName) {
      return false;
    }
  } else {
    return false;
  }

  // perform actual checking the command:
  if (auto [success, handle] = ParseCreateClassResponse(data, idx); success) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << kLogTag
                            << ": Server successfully created a CustomClass instance with handle: "
                            << handle));
    if (!ClassRepository::GetInstance().RegisterClassHandle(handle)) {
      Logger::LogError(
          Logger::to_string(std::stringstream()
                            << kLogTag << ": Can't register the class handle: " << handle << "!"));
    }
    return true;
  } else {
    Logger::LogError(Logger::to_string(std::stringstream() << kLogTag << ": Can't parse data!"));
  }

  return false;
}

std::pair<bool, ClassHandle> ResponseParser::ParseCreateClassResponse(const RawDataType& data,
                                                                      size_t& seek_idx) const {
  auto idx = seek_idx;
  if (data.size() < idx + 2 || (data[idx++] != '#' || data[idx++] != 'i')) {  // 'i' - instance
    return std::make_pair(false, -1);
  }

  if (auto [success, handle] = RegularTypeParaser::Parse<int>(data, idx); success) {
    return std::make_pair(true, handle);
  }

  return std::make_pair(false, -1);
}

RequestId ResponseParser::ParseRequestId(const RawDataType& request, size_t& seek_idx) const {
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

bool ResponseParser::RegisterRequest(RequestId request_id, ClientRequest request) {
  std::lock_guard<std::mutex> locker(requests_mutex_);

  if (requests_.count(request_id) > 0) {
    return false;
  }

  requests_[request_id] = request;
  return true;
}