#pragma once

#include "ome/types.hpp"

#include <array>
#include <cstdint>

namespace ome {

enum class MessageType : std::uint8_t {
  NewOrder = 1,
  Cancel = 2,
  Amend = 3
};

enum class EventType : std::uint8_t {
  Ack = 1,
  Reject = 2,
  Trade = 3,
  CancelAck = 4,
  AmendAck = 5
};

/// Fixed-size ingress message — no heap, no strings (hot-path safe).
struct IncomingMessage {
  MessageType type{MessageType::NewOrder};
  OrderId order_id{kInvalidOrderId};
  OrderId client_order_id{kInvalidOrderId};
  Side side{Side::Buy};
  OrderType order_type{OrderType::Limit};
  TimeInForce tif{TimeInForce::Day};
  Price price{0};
  Qty qty{0};
  TimestampNs ingress_ts_ns{0};
};

/// Fixed-size outbound event.
struct OutboundEvent {
  EventType type{EventType::Ack};
  OrderId order_id{kInvalidOrderId};
  OrderId aggressor_id{kInvalidOrderId};
  OrderId resting_id{kInvalidOrderId};
  Side side{Side::Buy};
  Price price{0};
  Qty qty{0};
  Qty leaves_qty{0};
  RejectReason reason{RejectReason::None};
  TimestampNs ts_ns{0};
  SeqNum seq{0};
};

}  // namespace ome
