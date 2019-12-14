#include "CustomClass.h"

#include <iostream>
#include <sstream>
#include "DataDeserializer.h"
#include "DataSerializer.h"
#include "Logger.h"

constexpr auto kLogTag = "CustomClass";

const std::string CustomClass::kClassName = typeid(CustomClass).name();

CustomClass::CustomClass() {
  Logger::LogDebug(Logger::to_string(
      std::stringstream() << kLogTag << ": created default CustomClass, this=" << this));
}

CustomClass::CustomClass(int ival) : ival_(ival) {
  Logger::LogDebug(Logger::to_string(std::stringstream()
                                     << kLogTag << ": created CustomClass(int), this=" << this));
}

CustomClass::CustomClass(int ival, std::string str) : ival_(ival), str_(std::move(str)) {
  Logger::LogDebug(Logger::to_string(
      std::stringstream() << kLogTag << ": created CustomClass(int, std::string), this=" << this));
}

void CustomClass::PrintToCout() const {
  Logger::LogDebug(Logger::to_string(std::stringstream() << kLogTag << ": " << PrintToString()));
}

std::string CustomClass::PrintToString() const {
  std::stringstream ss;
  ss << "CustomClass::this=" << this << "\n"
     << "CustomClass::ival_=" << ival_ << "\n"
     << "CustomClass::str_=" << str_;

  return ss.str();
}

bool CustomClass::SetIntegerValue(int ival) {
  if (ival == ival_) {
    return false;
  }

  ival_ = ival;
  Logger::LogDebug(Logger::to_string(std::stringstream()
                                     << kLogTag << ": change CustomClass::ival_ to=" << ival_));
  return true;
}

bool CustomClass::SetStringValue(std::string str) {
  if (str == str_) {
    return false;
  }

  str_ = std::move(str);
  Logger::LogDebug(Logger::to_string(std::stringstream()
                                     << kLogTag << ": change CustomClass::str_ to=" << str_));
  return true;
}

std::string CustomClass::Serialize() const {
  std::stringstream ss;
  DataSerializer::Serialize<int>(ss, this->ival_);
  DataSerializer::Serialize<std::string>(ss, this->str_);
  return ss.str();
}
CustomClass CustomClass::Deserialize(const std::string& serialized) {
  RawDataType data = RawDataType{serialized.begin(), serialized.end()};

  int ival = 0;
  size_t seek_index = 0;
  // Try to parse integer attribute of a class
  if (auto [success, value] = RegularTypeParaser::Parse<int>(data, seek_index); success) {
    ival = value;
  } else {
    return CustomClass{};
  }

  // Try to parse string attribute of a class
  std::string str;
  if (auto [success, value] = RegularTypeParaser::Parse<std::string>(data, seek_index); success) {
    str = std::move(value);
  } else {
    return CustomClass{};
  }

  return CustomClass(ival, std::move(str));
}
