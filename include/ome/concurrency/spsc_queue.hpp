#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ome {

/// Wait-free SPSC ring buffer. Capacity must be a power of two.
/// Complexity: try_push / try_pop O(1).
template <typename T>
class SpscQueue {
 public:
  explicit SpscQueue(std::size_t capacity)
      : capacity_(next_pow2(capacity)),
        mask_(capacity_ - 1),
        buffer_(capacity_) {
    if (capacity_ < 2) {
      throw std::invalid_argument("SpscQueue capacity must be >= 2");
    }
  }

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

  /// Producer only. Complexity: O(1).
  [[nodiscard]] bool try_push(const T& item) noexcept {
    const auto head = head_.load(std::memory_order_relaxed);
    const auto next = (head + 1) & mask_;
    if (next == tail_.load(std::memory_order_acquire)) {
      return false;  // full
    }
    buffer_[static_cast<std::size_t>(head)] = item;
    head_.store(next, std::memory_order_release);
    return true;
  }

  /// Producer only. Complexity: O(1).
  [[nodiscard]] bool try_push(T&& item) noexcept {
    const auto head = head_.load(std::memory_order_relaxed);
    const auto next = (head + 1) & mask_;
    if (next == tail_.load(std::memory_order_acquire)) {
      return false;
    }
    buffer_[static_cast<std::size_t>(head)] = std::move(item);
    head_.store(next, std::memory_order_release);
    return true;
  }

  /// Consumer only. Complexity: O(1).
  [[nodiscard]] bool try_pop(T& out) noexcept {
    const auto tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) {
      return false;  // empty
    }
    out = std::move(buffer_[static_cast<std::size_t>(tail)]);
    tail_.store((tail + 1) & mask_, std::memory_order_release);
    return true;
  }

 private:
  static std::size_t next_pow2(std::size_t n) {
    if (n < 2) {
      return 2;
    }
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  const std::size_t capacity_;
  const std::size_t mask_;
  std::vector<T> buffer_;

  alignas(64) std::atomic<std::uint64_t> head_{0};
  alignas(64) std::atomic<std::uint64_t> tail_{0};
};

}  // namespace ome
