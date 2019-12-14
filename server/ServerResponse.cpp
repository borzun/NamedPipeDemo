#include "ServerResponse.h"

ServerResponse::ServerResponse(RawDataType data, SuccessCallbackType success_callback,
                               FailureCallbackType failure_callback)
    : data_(std::move(data)),
      success_callback_(std::move(success_callback)),
      failure_callback_(std::move(failure_callback)) {}

bool ServerResponse::IsValid() const { return !data_.empty(); }

RawDataType ServerResponse::GetData() const { return data_; }

void ServerResponse::HandleSuccess(ClientId client_id) {
  if (success_callback_) {
    success_callback_(client_id);
  }
}

void ServerResponse::HandleFailure(ClientId client_id, ErrorCode error) {
  if (failure_callback_) {
    failure_callback_(client_id, error);
  }
}
