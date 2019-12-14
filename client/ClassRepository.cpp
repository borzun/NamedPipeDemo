#include "ClassRepository.h"

#include <algorithm>

ClassRepository& ClassRepository::GetInstance() {
  static ClassRepository instance;
  return instance;
}

bool ClassRepository::RegisterClassHandle(ClassHandle handle) {
  if (ContainsClassHandle(handle)) {
    return false;
  }

  {
    std::lock_guard<std::mutex> locker(mutex_);
    handles_.push_back(handle);
  }
  return true;
}

bool ClassRepository::ContainsClassHandle(ClassHandle handle) const {
  std::lock_guard<std::mutex> locker(mutex_);
  return std::find(handles_.begin(), handles_.end(), handle) != handles_.end();
}
