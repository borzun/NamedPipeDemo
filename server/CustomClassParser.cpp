#include "CustomClassParser.h"

#include <sstream>
#include "ClassRegistry.h"
#include "DataDeserializer.h"
#include "DataSerializer.h"
#include "Logger.h"

const std::string CustomClassParser::kClassName = typeid(CustomClass).name();

static const std::string kPrintToCoutMethodName = "PrintToCout";
static const std::string kPrintToStringMethodName = "PrintToString";
static const std::string kSetIntegerValMethodName = "SetIntegerValue";
static const std::string kSetStringValueMethodName = "SetStringValue";

static const auto kInvalidResponse = std::make_pair(false, ServerResponse{});
static const auto kInvalidHandlePair = std::make_pair(false, -1);

static constexpr auto kLogTag = "CustomClassParser";

CustomClassParser::CustomClassParser(size_t client_id, RequestId request_id)
    : client_id_(client_id), request_id_(request_id) {}

std::pair<bool, ServerResponse> CustomClassParser::Parse(const RawDataType& data, size_t& idx) {
  if (data.empty() || data[idx++] != '#') {
    return kInvalidResponse;
  }

  // Check that class name is the same as the CustomClass:
  if (auto [success, str] = RegularTypeParaser::Parse<std::string>(data, idx); success) {
    if (str != kClassName) {
      return kInvalidResponse;
    }
  } else {
    return kInvalidResponse;
  }

  // perform actual checking the command:
  if (auto [success, handle] = ParseCreateClass(data, idx); success) {
    return std::make_pair(true, CreateServerResponseOnCreateClassRequest(handle));
  }
  // All other commands requires to use the instance handle:
  auto [handle, instance] = GetClassInstaceFromRequest(data, idx);
  if (!instance) {
    Logger::LogError(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id_
                            << "] ERROR - invalid handle of CustomClass=" << handle << "!"));
    return kInvalidResponse;
  }

  // Try to parse method call (#m keyword)
  if (auto [success, response_data, method_name] = ParseMethodCall(*instance.get(), data, idx);
      success) {
    return std::make_pair(true,
                          CreateServerResponseOnMethodCall(handle, response_data, method_name));
  } else if (auto [success, response_data] = ParseGetInstance(*instance.get(), data, idx);
             success) {
    return std::make_pair(true, ServerResponse(response_data, nullptr, nullptr));
  }

  Logger::LogError(Logger::to_string(std::stringstream()
                                     << kLogTag << ": [client=" << client_id_ << ", request="
                                     << request_id_ << "] ERROR - Cannot process the request!"));
  return std::make_pair(true, ServerResponse{});
}

std::pair<bool, ClassHandle> CustomClassParser::ParseCreateClass(const RawDataType& data,
                                                                 size_t& seek_idx) {
  size_t tmp_idx = seek_idx;
  // verify that current operation is create operation (#c)
  if (data.size() < tmp_idx + 2 || (data[tmp_idx++] != '#' || data[tmp_idx++] != 'c')) {
    return kInvalidHandlePair;
  }

  // no attributes given - call default ctor
  if (data.size() == tmp_idx) {
    seek_idx = tmp_idx;
    auto handle = ClassRegistry<CustomClass>::GetInstance().Create();
    return std::make_pair(true, handle);
  }

  // Parse all attributes
  // TODO - think about how to pass variable vector of types (tuple/ vector of
  // any) into variadic template function!
  int ival = 0;
  if (auto [success, val] = RegularTypeParaser::Parse<int>(data, tmp_idx); success) {
    ival = val;
  } else {
    return kInvalidHandlePair;
  }

  // passed only one argument to ctor:
  if (data.size() == tmp_idx) {
    seek_idx = tmp_idx;
    auto handle = ClassRegistry<CustomClass>::GetInstance().Create(ival);
    return std::make_pair(true, handle);
  }

  // try to parse second argument (string)
  std::string str;
  if (auto [success, val] = RegularTypeParaser::Parse<std::string>(data, tmp_idx); success) {
    str = std::move(val);
  } else {
    return kInvalidHandlePair;
  }

  if (data.size() == tmp_idx) {
    seek_idx = tmp_idx;
    auto handle = ClassRegistry<CustomClass>::GetInstance().Create(ival, str);
    return std::make_pair(true, handle);
  }

  return kInvalidHandlePair;
}

std::tuple<bool, RawDataType, std::string> CustomClassParser::ParseMethodCall(
    CustomClass& instance, const RawDataType& data, size_t& seek_idx) {
  size_t tmp_idx = seek_idx;
  if (!IsMethodCall(data, tmp_idx)) {
    return std::make_tuple(false, RawDataType{}, std::string{});
  }

  seek_idx = tmp_idx;

  // Get actual method name:
  auto [success, method_name] = RegularTypeParaser::Parse<std::string>(data, seek_idx);
  if (!success) {
    Logger::LogError(Logger::to_string(
        std::stringstream() << kLogTag << ": [client=" << client_id_ << ", request=" << request_id_
                            << "] ERROR - can't parse the method's name!"));
    return std::make_tuple(true, RawDataType{}, std::string{});
  }

  if (method_name == kPrintToCoutMethodName) {
    ParsePrintToCoutCall(instance);
    return std::make_tuple(true, RawDataType{}, kPrintToCoutMethodName);
  } else if (method_name == kPrintToStringMethodName) {
    auto ret = ParsePrintToStringCall(instance);
    auto data = DataSerializer::SerializeToRawData<std::string>(ret);
    return std::make_tuple(true, data, kPrintToStringMethodName);
  } else if (method_name == kSetIntegerValMethodName) {
    auto ret = ParseSetIntegerValueMethodCall(instance, data, seek_idx);
    // TODO:
    // Check if ret.first is successfull. If not, return error
    auto data = DataSerializer::SerializeToRawData<bool>(ret.second);
    return std::make_tuple(ret.first, data, kSetIntegerValMethodName);
  } else if (method_name == kSetStringValueMethodName) {
    // TODO:
    // Check if ret.first is successfull. If not, return error
    auto ret = ParseSetStringValue(instance, data, seek_idx);
    auto data = DataSerializer::SerializeToRawData<bool>(ret.second);
    return std::make_tuple(ret.first, data, kSetIntegerValMethodName);
  }

  return std::make_tuple(true, RawDataType{}, std::string{});
}

std::pair<bool, RawDataType> CustomClassParser::ParseGetInstance(CustomClass& instance,
                                                                 const RawDataType& data,
                                                                 size_t& seek_idx) {
  size_t idx = seek_idx;
  // check for keyword get instance - 'g':
  if (data.size() < idx + 2 || (data[idx++] != '#' || data[idx++] != 'g')) {
    return std::make_pair(false, RawDataType{});
  }

  std::string str = instance.Serialize();
  RawDataType response_data = DataSerializer::SerializeToRawData(str);
  return std::make_pair(true, std::move(response_data));
}

void CustomClassParser::ParsePrintToCoutCall(CustomClass& instance) {
  return instance.PrintToCout();
}

std::string CustomClassParser::ParsePrintToStringCall(CustomClass& instance) {
  return instance.PrintToString();
}

std::pair<bool, bool> CustomClassParser::ParseSetIntegerValueMethodCall(CustomClass& instance,
                                                                        const RawDataType& data,
                                                                        size_t& seek_idx) {
  int ival = 0;
  if (auto [success, val] = RegularTypeParaser::Parse<int>(data, seek_idx); success) {
    ival = val;
  } else {
    return std::make_pair(false, false);
  }

  const bool ret = instance.SetIntegerValue(ival);
  return std::make_pair(true, ret);
}

std::pair<bool, bool> CustomClassParser::ParseSetStringValue(CustomClass& instance,
                                                             const RawDataType& data,
                                                             size_t& seek_idx) {
  std::string str;
  if (auto [success, s] = RegularTypeParaser::Parse<std::string>(data, seek_idx); success) {
    str = std::move(s);
  } else {
    return std::make_pair(false, false);
  }

  const bool ret = instance.SetStringValue(str);
  return std::make_pair(true, ret);
}

bool CustomClassParser::IsMethodCall(const RawDataType& data, size_t& seek_idx) {
  size_t tmp_idx = seek_idx;
  // verify that current operation is method call operation (#m)
  if (data.size() < tmp_idx + 2 || (data[tmp_idx++] != '#' || data[tmp_idx++] != 'm')) {
    return false;
  }
  seek_idx = tmp_idx;
  return true;
}

std::pair<ClassHandle, std::shared_ptr<CustomClass>> CustomClassParser::GetClassInstaceFromRequest(
    const RawDataType& data, size_t& seek_idx) {
  auto handle = RegularTypeParaser::Parse<ClassHandle>(data, seek_idx).second;
  auto instance = ClassRegistry<CustomClass>::GetInstance().GetClassObjectByHandle(handle);
  return std::make_pair(handle, instance.lock());
}

ServerResponse CustomClassParser::CreateServerResponseOnCreateClassRequest(ClassHandle handle) {
  std::stringstream ss;
  DataSerializer::Serialize<ClassHandle>(ss, handle);

  std::string str = ss.str();
  using IterType = decltype(str.begin());
  auto data = RawDataType{std::move_iterator<IterType>(str.begin()),
                          std::move_iterator<IterType>(str.end())};

  auto req_id = request_id_;
  auto success_callback = [req_id, handle](ServerResponse::ClientId client_id) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << "Successfully sent response to client=" << client_id
                            << " on creating the class: " << handle << "; request_id=" << req_id));
  };

  auto failure_callback = [req_id, handle](ServerResponse::ClientId client_id,
                                           ServerResponse::ErrorCode error) {
    Logger::LogDebug(Logger::to_string(std::stringstream()
                                       << "Failed to send response to client=" << client_id
                                       << " on creating the class: " << handle
                                       << ", error=" << error << "; request_id=" << req_id));
  };

  return ServerResponse(data, success_callback, failure_callback);
}

ServerResponse CustomClassParser::CreateServerResponseOnMethodCall(
    ClassHandle handle, RawDataType data, const std::string& method_name) {
  auto req_id = request_id_;
  auto success_callback = [req_id, handle, method_name](ServerResponse::ClientId client_id) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << "Successfully sent response to client=" << client_id
                            << " on call method=" << method_name
                            << " on class with handle=" << handle << "; request_id=" << req_id));
  };

  auto failure_callback = [req_id, handle, method_name](ServerResponse::ClientId client_id,
                                                        ServerResponse::ErrorCode error) {
    Logger::LogDebug(Logger::to_string(
        std::stringstream() << "Failed to send response to client=" << client_id
                            << " on call method=" << method_name << " on class with handle="
                            << handle << ", error=" << error << "; request_id=" << req_id));
  };

  return ServerResponse(data, success_callback, failure_callback);
}