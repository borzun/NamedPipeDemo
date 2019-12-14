#pragma once

#include <functional>
#include "Types.h"

// Class, which encapsulate the response from the server on the client's
// operation Usually constructed with 3 parameters:
//  - success_callback - callback which will be called when server successfully
//  send response back to client
//  - failure_callback - callback which will be called when server failed to
//  send response back to client.
//                       this callback will accept the GLE as an input
//                       parameter.
//  - data - data, which will be send back to the user
class ServerResponse final {
 public:
  using ClientId = int;
  using ErrorCode = int;

  // first paramter - client id
  using SuccessCallbackType = std::function<void(ClientId)>;
  // first paramter - client_id, second - error
  using FailureCallbackType = std::function<void(ClientId, ErrorCode)>;

 public:
  // Constructs invalid response - means nothing should be sent back to the
  // client.
  ServerResponse() = default;
  // Valid response to client
  ServerResponse(RawDataType data, SuccessCallbackType success_callback,
                 FailureCallbackType failure_callback);

  bool IsValid() const;

  RawDataType GetData() const;

  // Called when response is successfully sent.
  void HandleSuccess(ClientId client_id);
  // Invoked when response is failed to sent to client.
  void HandleFailure(ClientId client_id, ErrorCode error);

  inline void SetRequestId(RequestId request_id) { request_id_ = request_id; }

  inline RequestId GetRequestId() const { return request_id_; }

 private:
  RawDataType data_;
  SuccessCallbackType success_callback_;
  FailureCallbackType failure_callback_;
  RequestId request_id_ = -1;
};