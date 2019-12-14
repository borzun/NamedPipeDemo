#pragma once

#include <any>
#include <functional>
#include "Types.h"

// Class which encapsulate the user's request.
// Also, it will process the server's response on user's request.
// So, you can provide the callbacks, which will handle success or failed state
// of server's response.
class ClientRequest final {
 public:
  using SuccessCallbackType = std::function<void(std::any)>;
  using FailureCallbackType = std::function<void(int)>;

 public:
  ClientRequest() = default;

  explicit ClientRequest(RawDataType data, bool wait_for_response = false);
  ClientRequest(RawDataType data, bool wait_for_response, SuccessCallbackType succes_callback,
                FailureCallbackType failure_callback);

  inline bool NeedToWaitForResponse() const { return wait_for_response_; }

  // TODO: maybe avoid copying?
  inline RawDataType GetData() const { return data_; }

  // Invoke the response object with successful result
  void HandleSuccess(std::any result);
  // Invoke the response object with failed result and GLE
  void HandleFailure(int error);

 private:
  RawDataType data_;
  bool wait_for_response_ = false;
  SuccessCallbackType succes_callback_;
  FailureCallbackType failure_callback_;
};