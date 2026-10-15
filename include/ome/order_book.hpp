#pragma once

#include "ome/memory/object_pool.hpp"
#include "ome/order.hpp"
#include "ome/price_level.hpp"
#include "ome/protocol/message.hpp"
#include "ome/types.hpp"

#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

namespace ome {

/// Single-instrument limit order book with price-time priority.
/// Bids keyed descending (std::greater), asks ascending.
class OrderBook {
 public:
  using TradeCallback = std::function<void(const OutboundEvent&)>;

  explicit OrderBook(std::size_t order_pool_capacity);

  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

  /// Rest an order (already validated, no match). Complexity: O(1) amorti + O(log P).
  [[nodiscard]] bool add_resting(Order* order);

  /// Match aggressor against opposite book; leftover may rest if limit+Day.
  /// Complexity: O(k) fills (+ O(log P) per emptied level).
  Qty match(Order& aggressor, TradeCallback on_trade);

  /// Cancel by id. Complexity: O(1) amorti (+ O(log P) if level emptied).
  [[nodiscard]] bool cancel(OrderId id, OutboundEvent& ack);

  /// Amend qty↓ same price in-place O(1); else cancel+reinsert (loses time priority).
  [[nodiscard]] bool amend(OrderId id, Price new_price, Qty new_qty, OutboundEvent& ack,
                           TradeCallback on_trade);

  [[nodiscard]] Price best_bid() const noexcept { return best_bid_; }
  [[nodiscard]] Price best_ask() const noexcept { return best_ask_; }
  [[nodiscard]] std::size_t order_count() const noexcept { return index_.size(); }
  [[nodiscard]] bool empty() const noexcept { return index_.empty(); }

  [[nodiscard]] Order* find(OrderId id) const {
    auto it = index_.find(id);
    return it == index_.end() ? nullptr : it->second;
  }

  [[nodiscard]] ObjectPool<Order>& pool() noexcept { return pool_; }

 private:
  using BidLevels = std::map<Price, PriceLevel, std::greater<Price>>;
  using AskLevels = std::map<Price, PriceLevel, std::less<Price>>;

  void refresh_tops() noexcept;
  void erase_level_if_empty(Side side, Price price);
  PriceLevel* get_or_create_level(Side side, Price price);
  PriceLevel* find_level(Side side, Price price);

  ObjectPool<Order> pool_;
  BidLevels bids_;
  AskLevels asks_;
  std::unordered_map<OrderId, Order*> index_;
  Price best_bid_{kInvalidPrice};
  Price best_ask_{kInvalidPrice};
};

}  // namespace ome
