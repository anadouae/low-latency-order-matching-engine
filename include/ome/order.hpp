#pragma once

#include "ome/types.hpp"

namespace ome {

/// Fixed-size resting order. Target ≤ 64 bytes (1 cache line) for hot-path locality.
/// Complexity: node unlink/link is O(1) when `prev`/`next` are known.
struct alignas(8) Order {
  OrderId id{kInvalidOrderId};
  Price price{0};
  Qty qty{0};          // original
  Qty remaining{0};    // unfilled
  Side side{Side::Buy};
  OrderType type{OrderType::Limit};
  TimeInForce tif{TimeInForce::Day};
  std::uint8_t flags{0};
  TimestampNs ts_ns{0};
  Order* prev{nullptr};
  Order* next{nullptr};

  [[nodiscard]] bool is_active() const noexcept { return remaining > 0; }
};

static_assert(sizeof(Order) <= 64, "Order should fit in one cache line");

}  // namespace ome
