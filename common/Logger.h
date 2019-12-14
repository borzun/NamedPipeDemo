#pragma once

#include <iostream>
#include <string>

// Logger class to log messages to std::cout or std::cerr
// This class also avoids the torn writes to a stream,
// cause both server and client can have multiple threads to write to the log.
// Thus, we need to have some kind of synchronization.
class Logger final {
 public:
  static void LogDebug(const std::string& message);

  static void LogError(const std::string& error);

  // Helper method to convert std::ostream to std::string
  // this is just helper for stringstream one liners:
  // For more details, see https://github.com/stan-dev/math/issues/590
  static std::string to_string(std::ostream& stream);
};
