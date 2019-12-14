#pragma once

#include <mutex>
#include <vector>
#include "Types.h"

// Class to manage the instances of current CustomClass objects created on the
// server
class ClassRepository {
 public:
  static ClassRepository& GetInstance();

  bool RegisterClassHandle(ClassHandle handle);
  bool ContainsClassHandle(ClassHandle handle) const;

  inline std::vector<ClassHandle> GetAllHandles() const { return handles_; }

 private:
  // methods of this class can be called both sync and async.
  // so, this means that it can be called from different theads.
  // Thus, synchronization needed.
  mutable std::mutex mutex_;
  std::vector<ClassHandle> handles_;
};