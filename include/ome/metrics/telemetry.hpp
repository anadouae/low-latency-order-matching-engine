#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>

namespace ome {

/// Lightweight telemetry hooks. No heap, safe from matching thread.
/// Histogram is a coarse fixed-bucket stub suitable for demos / CI.
class Telemetry {
 public:
  void record_latency_ns(std::uint64_t ns) noexcept {
    latency_samples_.fetch_add(1, std::memory_order_relaxed);
    latency_sum_ns_.fetch_add(ns, std::memory_order_relaxed);
    // 8 buckets: <1us, <2, <5, <10, <20, <50, <100, >=100
    const std::size_t bucket = bucket_for(ns);
    buckets_[bucket].fetch_add(1, std::memory_order_relaxed);
  }

  void incr_orders() noexcept { orders_.fetch_add(1, std::memory_order_relaxed); }
  void incr_cancels() noexcept { cancels_.fetch_add(1, std::memory_order_relaxed); }
  void incr_amends() noexcept { amends_.fetch_add(1, std::memory_order_relaxed); }
  void incr_trades() noexcept { trades_.fetch_add(1, std::memory_order_relaxed); }
  void incr_rejects() noexcept { rejects_.fetch_add(1, std::memory_order_relaxed); }

  [[nodiscard]] std::uint64_t orders() const noexcept {
    return orders_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t cancels() const noexcept {
    return cancels_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t amends() const noexcept {
    return amends_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t trades() const noexcept {
    return trades_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t rejects() const noexcept {
    return rejects_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t latency_samples() const noexcept {
    return latency_samples_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t latency_sum_ns() const noexcept {
    return latency_sum_ns_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t bucket(std::size_t i) const noexcept {
    return i < buckets_.size() ? buckets_[i].load(std::memory_order_relaxed) : 0;
  }

 private:
  static std::size_t bucket_for(std::uint64_t ns) noexcept {
    if (ns < 1'000) return 0;
    if (ns < 2'000) return 1;
    if (ns < 5'000) return 2;
    if (ns < 10'000) return 3;
    if (ns < 20'000) return 4;
    if (ns < 50'000) return 5;
    if (ns < 100'000) return 6;
    return 7;
  }

  std::atomic<std::uint64_t> orders_{0};
  std::atomic<std::uint64_t> cancels_{0};
  std::atomic<std::uint64_t> amends_{0};
  std::atomic<std::uint64_t> trades_{0};
  std::atomic<std::uint64_t> rejects_{0};
  std::atomic<std::uint64_t> latency_samples_{0};
  std::atomic<std::uint64_t> latency_sum_ns_{0};
  std::array<std::atomic<std::uint64_t>, 8> buckets_{};
};

}  // namespace ome
