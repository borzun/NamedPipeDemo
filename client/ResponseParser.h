#pragma once

#include <mutex>
#include <unordered_map>
#include <utility>
#include "ClientRequest.h"
#include "Types.h"

// Parser of server responses.
// NOTE: the virtual dtor and methods are added solely for future testing
// purposes by injecting corresponding mock objects.
class ResponseParser {
 public:
  ResponseParser() = default;
  virtual ~ResponseParser() = default;

  // Register the sent request. We expect that in future will receive a response
  // from that request.
  bool RegisterRequest(RequestId request_id, ClientRequest request);

  virtual bool ParseResponse(const RawDataType& data);

 private:
  RequestId ParseRequestId(const RawDataType& request, size_t& seek_idx) const;

  bool ParseCustomClassResponse(const RawDataType& data) const;
  std::pair<bool, ClassHandle> ParseCreateClassResponse(const RawDataType& data,
                                                        size_t& seek_idx) const;

 private:
  std::mutex requests_mutex_;
  std::unordered_map<RequestId, ClientRequest> requests_;
};