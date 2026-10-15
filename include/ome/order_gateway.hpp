#pragma once

#include "ome/config.hpp"
#include "ome/matching_engine.hpp"
#include "ome/protocol/message.hpp"
#include "ome/types.hpp"

#include <atomic>
#include <chrono>

namespace ome {

/// Validates and stamps incoming orders, then enqueues to the matching engine.
class OrderGateway {
 public:
  explicit OrderGateway(MatchingEngine& engine) : engine_(engine) {}

  /// Complexity: O(1) validation + O(1) enqueue attempt.
  [[nodiscard]] bool submit_new(OrderId id, Side side, Price price, Qty qty,
                                OrderType type = OrderType::Limit,
                                TimeInForce tif = TimeInForce::Day);

  [[nodiscard]] bool submit_cancel(OrderId id);
  [[nodiscard]] bool submit_amend(OrderId id, Price new_price, Qty new_qty);

  [[nodiscard]] std::uint64_t rejected() const noexcept {
    return rejected_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t accepted() const noexcept {
    return accepted_.load(std::memory_order_relaxed);
  }

 private:
  [[nodiscard]] RejectReason validate_new(OrderId id, Price price, Qty qty,
                                          OrderType type) const noexcept;
  static TimestampNs now_ns() noexcept;

  MatchingEngine& engine_;
  std::atomic<std::uint64_t> rejected_{0};
  std::atomic<std::uint64_t> accepted_{0};
};

}  // namespace ome
