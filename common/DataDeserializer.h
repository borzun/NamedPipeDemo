#pragma once

#include <any>
#include <utility>
#include "Types.h"

// Deserializer of regular types from raw data.
// Currently defined only for bool, int, double and std::string
class RegularTypeParaser {
 public:
  template <class Type>
  static std::pair<bool, Type> Parse(const RawDataType& data, size_t& seek_index);
};
