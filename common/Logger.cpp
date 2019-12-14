#include "Logger.h"

#include <iostream>
#include <mutex>
#include <sstream>

static std::mutex s_log_mutex;

void Logger::LogDebug(const std::string& message) {
  std::lock_guard<std::mutex> locker(s_log_mutex);

  std::cout << message << std::endl;
}

void Logger::LogError(const std::string& message) {
  std::lock_guard<std::mutex> locker(s_log_mutex);

  std::cerr << message << std::endl;
}

std::string Logger::to_string(std::ostream& stream) {
  return static_cast<std::stringstream&>(stream).str();
}