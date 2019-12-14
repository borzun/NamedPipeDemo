#pragma once

#include <any>
#include <tuple>
#include <typeinfo>
#include <utility>

#include "CustomClass.h"
#include "ServerResponse.h"
#include "Types.h"

// Class to parse the raw data of CustomClass object
class CustomClassParser {
 public:
  CustomClassParser(size_t client_id, RequestId request);

  std::pair<bool, ServerResponse> Parse(const RawDataType& data, size_t& idx);

  std::pair<bool, ClassHandle> ParseCreateClass(const RawDataType& data, size_t& seek_idx);

  // Return type - {success of operation, return value of method call, method
  // call name}
  std::tuple<bool, RawDataType, std::string> ParseMethodCall(CustomClass& instance,
                                                             const RawDataType& data,
                                                             size_t& seek_idx);

  // Parse getting the object:
  std::pair<bool, RawDataType> ParseGetInstance(CustomClass& instance, const RawDataType& data,
                                                size_t& seek_idx);

 public:
  static const std::string kClassName;

 private:
  // Methods to parse the method call request
  void ParsePrintToCoutCall(CustomClass& instance);

  std::string ParsePrintToStringCall(CustomClass& instance);

  std::pair<bool, bool> ParseSetIntegerValueMethodCall(CustomClass& instance,
                                                       const RawDataType& data, size_t& seek_idx);

  std::pair<bool, bool> ParseSetStringValue(CustomClass& instance, const RawDataType& data,
                                            size_t& seek_idx);

  // Aux method to check whether next str request from data flow is actually
  // method call.
  bool IsMethodCall(const RawDataType& data, size_t& seek_idx);

  // Aux method to parse the data from request into class handle (and pointer)
  std::pair<ClassHandle, std::shared_ptr<CustomClass>> GetClassInstaceFromRequest(
      const RawDataType& data, size_t& seek_idx);

 private:
  // response data:
  ServerResponse CreateServerResponseOnCreateClassRequest(ClassHandle handle);

  ServerResponse CreateServerResponseOnMethodCall(ClassHandle handle, RawDataType ret,
                                                  const std::string& method_name);

 private:
  const size_t client_id_ = -1;
  const RequestId request_id_ = -1;
};