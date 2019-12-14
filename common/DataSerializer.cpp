#include "DataSerializer.h"

template <>
void DataSerializer::Serialize<bool>(std::ostream& stream, const bool& value) {
  stream << "b" << (value ? "1" : "0");
}

template <>
void DataSerializer::Serialize<int>(std::ostream& stream, const int& value) {
  constexpr auto kIntSize = sizeof(int);
  union CastIntToRawData {
    int value;
    char raw[kIntSize];
  };

  CastIntToRawData u;
  u.value = value;
  stream << "i";

  for (int i = 0; i < kIntSize; ++i) {
    stream << u.raw[i];
  }
}

template <>
void DataSerializer::Serialize<double>(std::ostream& stream, const double& value) {
  constexpr auto kDoubleSize = sizeof(double);
  union CastDoubleToRawData {
    double value;
    char raw[kDoubleSize];
  };

  CastDoubleToRawData u;
  u.value = value;

  stream << "d";

  for (int i = 0; i < kDoubleSize; ++i) {
    stream << u.raw[i];
  }
}

template <>
void DataSerializer::Serialize<std::string>(std::ostream& stream, const std::string& value) {
  stream << "s";
  Serialize<int>(stream, value.size());

  for (auto ch : value) {
    stream << ch;
  }
}

RawDataType DataSerializer::ConvertToRawData(std::string str) {
  using IterType = decltype(str.begin());
  return RawDataType{std::move_iterator<IterType>(str.begin()),
                     std::move_iterator<IterType>(str.end())};
}

std::string DataSerializer::ConvertRawDataToString(const RawDataType& data) {
  return std::string{data.begin(), data.end()};
}