#pragma once

#include <string>

class CustomClass {
 public:
  static const std::string kClassName;

 public:
  CustomClass();
  CustomClass(int ival);
  CustomClass(int ival, std::string str);

  // Print all attributes and their values to the std::cout
  void PrintToCout() const;

  // Print all attributes and their values to the std::string
  std::string PrintToString() const;

  // Set integer value to some new value.
  // If new value differs from old value, return true, false - otherwise
  bool SetIntegerValue(int ival);

  // Set string value to some new value.
  // If new value differs from old value, return true, false - otherwise
  bool SetStringValue(std::string str);

  std::string Serialize() const;
  static CustomClass Deserialize(const std::string& serialized);

 public:
  // Accessible fields:
  int ival_ = 0;
  std::string str_;
};
