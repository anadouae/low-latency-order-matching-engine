#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ome {

/// Pre-allocated object pool. Zero `new` in the matching loop once warm.
/// Complexity: acquire/release O(1) amorti (free-list pop/push).
template <typename T>
class ObjectPool {
 public:
  explicit ObjectPool(std::size_t capacity) {
    if (capacity == 0) {
      throw std::invalid_argument("ObjectPool capacity must be > 0");
    }
    storage_.resize(capacity);
    free_list_.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i) {
      free_list_.push_back(&storage_[i]);
    }
  }

  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }
  [[nodiscard]] std::size_t available() const noexcept { return free_list_.size(); }
  [[nodiscard]] bool empty() const noexcept { return free_list_.empty(); }

  /// Complexity: O(1). Returns nullptr if exhausted.
  [[nodiscard]] T* try_acquire() noexcept {
    if (free_list_.empty()) {
      return nullptr;
    }
    T* obj = free_list_.back();
    free_list_.pop_back();
    return obj;
  }

  /// Complexity: O(1). Throws if exhausted.
  [[nodiscard]] T* acquire() {
    T* obj = try_acquire();
    if (obj == nullptr) {
      throw std::bad_alloc();
    }
    return obj;
  }

  /// Complexity: O(1).
  void release(T* obj) noexcept {
    if (obj == nullptr) {
      return;
    }
    free_list_.push_back(obj);
  }

 private:
  std::vector<T> storage_;
  std::vector<T*> free_list_;
};

}  // namespace ome
