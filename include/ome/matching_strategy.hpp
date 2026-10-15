#pragma once

#include "ome/order.hpp"
#include "ome/order_book.hpp"
#include "ome/protocol/message.hpp"

namespace ome {

/// Price-time priority matching policy (exchange-standard FIFO within level).
class PriceTimePriorityStrategy {
 public:
  /// Apply a new order against the book. Returns leaves qty after match/rest.
  /// Complexity: O(k) match + O(log P) rest if leftover.
  static Qty apply_new(OrderBook& book, Order& order, OrderBook::TradeCallback on_trade);
};

}  // namespace ome
