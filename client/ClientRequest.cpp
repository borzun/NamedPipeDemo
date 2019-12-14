#include "ClientRequest.h"

ClientRequest::ClientRequest(RawDataType data, bool wait_for_response)
    : ClientRequest(std::move(data), wait_for_response, nullptr, nullptr) {}

ClientRequest::ClientRequest(RawDataType data, bool wait_for_response,
                             SuccessCallbackType succes_callback,
                             FailureCallbackType failure_callback)
    : data_(std::move(data)),
      wait_for_response_(wait_for_response),
      succes_callback_(succes_callback),
      failure_callback_(failure_callback) {}
void ClientRequest::HandleSuccess(std::any result) {
  if (succes_callback_) {
    succes_callback_(std::move(result));
  }
}

void ClientRequest::HandleFailure(int error) {
  if (failure_callback_) {
    failure_callback_(error);
  }
}
