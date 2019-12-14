#include "DataDeserializer.h"

template <>
std::pair<bool, bool> RegularTypeParaser::Parse(const RawDataType& data, size_t& seek_index) {
  constexpr auto kBoolSize = sizeof(bool);

  if (data[seek_index] != 'b' || (data.size() < seek_index + kBoolSize)) {
    return std::make_pair(false, false);
  }
  ++seek_index;
  char ch = data[seek_index];
  return std::make_pair(true, ch == '1');
}

template <>
std::pair<bool, int> RegularTypeParaser::Parse(const RawDataType& data, size_t& seek_index) {
  constexpr auto kIntSize = sizeof(int);

  if (data[seek_index] != 'i' || (data.size() < seek_index + kIntSize)) {
    return std::make_pair(false, int{});
  }

  union CastToInt {
    int val;
    char aux[kIntSize];
  };

  ++seek_index;
  CastToInt u;
  for (int i = 0; i < kIntSize; ++i) {
    u.aux[i] = data[seek_index++];
  }

  return std::make_pair(true, u.val);
}

template <>
std::pair<bool, double> RegularTypeParaser::Parse(const RawDataType& data, size_t& seek_index) {
  constexpr auto kDoubleSize = sizeof(double);

  if (data[seek_index] != 'd' || (data.size() < seek_index + kDoubleSize)) {
    return std::make_pair(false, double{});
  }

  union CastToDouble {
    double val;
    char aux[kDoubleSize];
  };

  ++seek_index;
  CastToDouble u;
  for (int i = 0; i < kDoubleSize; ++i) {
    u.aux[i] = data[seek_index++];
  }

  return std::make_pair(true, u.val);
}

template <>
std::pair<bool, std::string> RegularTypeParaser::Parse(const RawDataType& data,
                                                       size_t& seek_index) {
  if (data[seek_index] != 's') {
    return std::make_pair(false, std::string{});
  }

  size_t idx = seek_index + 1;
  auto p = RegularTypeParaser::Parse<int>(data, idx);
  if (!p.first || data.size() < idx + p.second) {
    return std::make_pair(false, std::string{});
  }

  std::string str(p.second, 0);
  for (size_t i = 0; i < str.size(); ++i) {
    str[i] = data[idx++];
  }

  seek_index = idx;
  return std::make_pair(true, str);
}