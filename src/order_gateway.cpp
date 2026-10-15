#include "ome/order_gateway.hpp"

namespace ome {

bool OrderGateway::submit_new(OrderId id, Side side, Price price, Qty qty, OrderType type,
                              TimeInForce tif) {
  const RejectReason reason = validate_new(id, price, qty, type);
  if (reason != RejectReason::None) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  IncomingMessage msg{};
  msg.type = MessageType::NewOrder;
  msg.order_id = id;
  msg.client_order_id = id;
  msg.side = side;
  msg.order_type = type;
  msg.tif = tif;
  msg.price = price;
  msg.qty = qty;
  msg.ingress_ts_ns = now_ns();

  if (!engine_.enqueue(msg)) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  accepted_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

bool OrderGateway::submit_cancel(OrderId id) {
  if (id == kInvalidOrderId) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  IncomingMessage msg{};
  msg.type = MessageType::Cancel;
  msg.order_id = id;
  msg.ingress_ts_ns = now_ns();
  if (!engine_.enqueue(msg)) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  accepted_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

bool OrderGateway::submit_amend(OrderId id, Price new_price, Qty new_qty) {
  if (id == kInvalidOrderId || new_qty <= 0 || new_price < config::kMinPriceTick ||
      new_price > config::kMaxPrice) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  IncomingMessage msg{};
  msg.type = MessageType::Amend;
  msg.order_id = id;
  msg.price = new_price;
  msg.qty = new_qty;
  msg.ingress_ts_ns = now_ns();
  if (!engine_.enqueue(msg)) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  accepted_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

RejectReason OrderGateway::validate_new(OrderId id, Price price, Qty qty,
                                        OrderType type) const noexcept {
  if (id == kInvalidOrderId) {
    return RejectReason::InvalidQty;
  }
  if (qty <= 0 || qty > config::kMaxQty) {
    return RejectReason::InvalidQty;
  }
  if (type == OrderType::Limit) {
    if (price < config::kMinPriceTick || price > config::kMaxPrice) {
      return RejectReason::InvalidPrice;
    }
  }
  return RejectReason::None;
}

TimestampNs OrderGateway::now_ns() noexcept {
  using namespace std::chrono;
  return static_cast<TimestampNs>(
      duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

}  // namespace ome
