#pragma once

#include "ome/protocol/message.hpp"

namespace ome::fixtures {

inline IncomingMessage make_new(OrderId id, Side side, Price price, Qty qty) {
  IncomingMessage m{};
  m.type = MessageType::NewOrder;
  m.order_id = id;
  m.client_order_id = id;
  m.side = side;
  m.order_type = OrderType::Limit;
  m.tif = TimeInForce::Day;
  m.price = price;
  m.qty = qty;
  return m;
}

inline IncomingMessage make_cancel(OrderId id) {
  IncomingMessage m{};
  m.type = MessageType::Cancel;
  m.order_id = id;
  return m;
}

inline IncomingMessage make_amend(OrderId id, Price price, Qty qty) {
  IncomingMessage m{};
  m.type = MessageType::Amend;
  m.order_id = id;
  m.price = price;
  m.qty = qty;
  return m;
}

}  // namespace ome::fixtures
