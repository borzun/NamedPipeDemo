#pragma once

#include "ServerResponse.h"
#include "Types.h"

// This class is responsible for parsing requests from the client.
class RequestParser {
 public:
  explicit RequestParser(size_t client_id);

  bool ParseRequest(const RawDataType& request, ServerResponse& response) const;

 private:
  RequestId ParseRequestId(const RawDataType& request, size_t& seek_idx) const;

  const size_t client_id_ = -1;
};