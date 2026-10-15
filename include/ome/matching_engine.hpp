#pragma once

#include "ome/concurrency/mpsc_queue.hpp"
#include "ome/concurrency/spsc_queue.hpp"
#include "ome/config.hpp"
#include "ome/matching_strategy.hpp"
#include "ome/metrics/telemetry.hpp"
#include "ome/order_book.hpp"
#include "ome/protocol/message.hpp"

#include <atomic>
#include <chrono>
#include <thread>

namespace ome {

/// Single-writer matching core. Consumes MPSC ingress, mutates one OrderBook,
/// publishes events on an SPSC outbound queue.
class MatchingEngine {
 public:
  MatchingEngine(std::size_t pool_capacity = config::kDefaultOrderPoolCapacity,
                 std::size_t ingress_capacity = config::kDefaultIngressCapacity,
                 std::size_t egress_capacity = config::kDefaultEgressCapacity);

  ~MatchingEngine();

  MatchingEngine(const MatchingEngine&) = delete;
  MatchingEngine& operator=(const MatchingEngine&) = delete;

  void start();
  void stop();

  [[nodiscard]] bool enqueue(const IncomingMessage& msg) noexcept {
    return ingress_.try_push(msg);
  }

  [[nodiscard]] bool poll_event(OutboundEvent& out) noexcept { return egress_.try_pop(out); }

  [[nodiscard]] OrderBook& book() noexcept { return book_; }
  [[nodiscard]] const OrderBook& book() const noexcept { return book_; }
  [[nodiscard]] Telemetry& telemetry() noexcept { return telemetry_; }
  [[nodiscard]] const Telemetry& telemetry() const noexcept { return telemetry_; }
  [[nodiscard]] bool running() const noexcept {
    return running_.load(std::memory_order_acquire);
  }

  /// Process up to `max_messages` synchronously (useful in tests). Complexity: O(n * cost).
  std::size_t drain(std::size_t max_messages = 1024);

 private:
  void run_loop();
  void handle(const IncomingMessage& msg);
  void publish(OutboundEvent ev);
  static TimestampNs now_ns() noexcept;

  OrderBook book_;
  MpscQueue<IncomingMessage> ingress_;
  SpscQueue<OutboundEvent> egress_;
  Telemetry telemetry_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stop_requested_{false};
  std::thread worker_;
  SeqNum seq_{0};
};

}  // namespace ome
