#pragma once

#include <ostream>
#include <sstream>
#include <string>
#include "Types.h"

// Helper class to serialize data into raw vector format!
class DataSerializer {
 public:
  template <class Type>
  static void Serialize(std::ostream& stream, const Type& value);

  template <class Type>
  static RawDataType SerializeToRawData(const Type& value) {
    std::stringstream ss;
    Serialize<Type>(ss, value);
    return DataSerializer::ConvertToRawData(ss.str());
  }
  static RawDataType ConvertToRawData(std::string str);

  static std::string ConvertRawDataToString(const RawDataType& data);

 private:
  DataSerializer() = delete;
};