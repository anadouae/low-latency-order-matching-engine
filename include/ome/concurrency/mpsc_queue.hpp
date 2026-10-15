#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace ome {

/// Bounded MPSC ring: multiple producers, single consumer.
/// Uses a ticket sequence per slot (classic Lamport / LMAX-style).
/// Complexity: try_push / try_pop O(1) amorti under low contention.
template <typename T>
class MpscQueue {
 public:
  explicit MpscQueue(std::size_t capacity)
      : capacity_(next_pow2(capacity)),
        mask_(capacity_ - 1),
        slots_(capacity_) {
    if (capacity_ < 2) {
      throw std::invalid_argument("MpscQueue capacity must be >= 2");
    }
    for (std::size_t i = 0; i < capacity_; ++i) {
      slots_[i].seq.store(static_cast<std::uint64_t>(i), std::memory_order_relaxed);
    }
  }

  MpscQueue(const MpscQueue&) = delete;
  MpscQueue& operator=(const MpscQueue&) = delete;

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

  /// Any producer thread. Complexity: O(1) amorti.
  [[nodiscard]] bool try_push(const T& item) noexcept {
    std::uint64_t pos = head_.load(std::memory_order_relaxed);
    for (;;) {
      Slot& slot = slots_[static_cast<std::size_t>(pos & mask_)];
      const std::uint64_t seq = slot.seq.load(std::memory_order_acquire);
      const std::intptr_t dif =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
      if (dif == 0) {
        if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          slot.value = item;
          slot.seq.store(pos + 1, std::memory_order_release);
          return true;
        }
      } else if (dif < 0) {
        return false;  // full
      } else {
        pos = head_.load(std::memory_order_relaxed);
      }
    }
  }

  /// Single consumer only. Complexity: O(1).
  [[nodiscard]] bool try_pop(T& out) noexcept {
    const std::uint64_t pos = tail_.load(std::memory_order_relaxed);
    Slot& slot = slots_[static_cast<std::size_t>(pos & mask_)];
    const std::uint64_t seq = slot.seq.load(std::memory_order_acquire);
    const std::intptr_t dif =
        static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
    if (dif == 0) {
      out = std::move(slot.value);
      slot.seq.store(pos + capacity_, std::memory_order_release);
      tail_.store(pos + 1, std::memory_order_relaxed);
      return true;
    }
    return false;  // empty
  }

 private:
  struct alignas(64) Slot {
    std::atomic<std::uint64_t> seq{0};
    T value{};
  };

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
  std::vector<Slot> slots_;

  alignas(64) std::atomic<std::uint64_t> head_{0};
  alignas(64) std::atomic<std::uint64_t> tail_{0};
};

}  // namespace ome
