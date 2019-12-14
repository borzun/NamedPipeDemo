#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include "Types.h"

// Manager of custom class instances.
// In future, would be better to remove signleton pattern!
template <class Type>
class ClassRegistry {
 public:
  static ClassRegistry<Type>& GetInstance();

  template <typename... Args>
  ClassHandle Create(Args... args);

  std::weak_ptr<Type> GetClassObjectByHandle(ClassHandle handle);

 private:
  ClassRegistry() = default;

 private:
  // This class can be accessible from different Client threads. Thus, need to
  // provide a synchronization!
  mutable std::mutex mutex_;
  ClassHandle handle_counter_;
  std::unordered_map<ClassHandle, std::shared_ptr<Type>> instances_;
};

template <class Type>
ClassRegistry<Type>& ClassRegistry<Type>::GetInstance() {
  static ClassRegistry<Type> s_instance;
  return s_instance;
}

template <class Type>
template <typename... Args>
ClassHandle ClassRegistry<Type>::Create(Args... args) {
  auto instance = std::make_shared<Type>(args...);
  {
    std::lock_guard<std::mutex> locker(mutex_);
    const auto handle = handle_counter_++;

    instances_.insert({handle, instance});
    return handle;
  }
}

template <class Type>
std::weak_ptr<Type> ClassRegistry<Type>::GetClassObjectByHandle(ClassHandle handle) {
  std::lock_guard<std::mutex> locker(mutex_);
  auto iter = instances_.find(handle);
  if (iter == instances_.end()) {
    return std::weak_ptr<Type>{};
  }

  return iter->second;
}