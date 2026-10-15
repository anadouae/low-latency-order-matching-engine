#pragma once

#include <cstdint>
#include <limits>

namespace ome {

using OrderId = std::uint64_t;
using Price = std::int64_t;   // ticks (fixed-point); never floating-point in hot path
using Qty = std::int64_t;
using SeqNum = std::uint64_t;
using TimestampNs = std::uint64_t;

enum class Side : std::uint8_t { Buy = 0, Sell = 1 };

enum class OrderType : std::uint8_t { Limit = 0, Market = 1 };

enum class TimeInForce : std::uint8_t { Day = 0, Ioc = 1 };

enum class RejectReason : std::uint8_t {
  None = 0,
  InvalidQty,
  InvalidPrice,
  UnknownOrder,
  DuplicateOrder,
  BookFull,
  NotSupported
};

inline constexpr OrderId kInvalidOrderId = 0;
inline constexpr Price kInvalidPrice = std::numeric_limits<Price>::min();

[[nodiscard]] inline constexpr Side opposite(Side side) noexcept {
  return side == Side::Buy ? Side::Sell : Side::Buy;
}

}  // namespace ome
