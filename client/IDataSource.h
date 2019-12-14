#pragma once

#include <vector>
#include "ClientRequest.h"
#include "Types.h"

// Simple interface to communicate with data
class IDataSource {
 public:
  IDataSource() = default;
  virtual ~IDataSource() = default;

  virtual ClientRequest ReadRequest() = 0;
  virtual bool IsGood() const = 0;
};