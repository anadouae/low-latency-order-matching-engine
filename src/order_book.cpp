#include "ome/order_book.hpp"

#include <utility>

namespace ome {

OrderBook::OrderBook(std::size_t order_pool_capacity) : pool_(order_pool_capacity) {
  index_.reserve(order_pool_capacity);
}

bool OrderBook::add_resting(Order* order) {
  // Complexity: O(1) amorti hash insert + O(log P) map insert if new level.
  PriceLevel* level = get_or_create_level(order->side, order->price);
  if (level == nullptr) {
    return false;
  }
  level->push_back(order);
  index_[order->id] = order;
  refresh_tops();
  return true;
}

Qty OrderBook::match(Order& aggressor, TradeCallback on_trade) {
  // Complexity: O(k) where k = number of resting orders touched.
  Qty filled = 0;

  auto walk = [&](auto& levels, auto crosses) {
    while (aggressor.remaining > 0 && !levels.empty()) {
      auto it = levels.begin();
      PriceLevel& level = it->second;
      if (!crosses(aggressor.price, level.price())) {
        break;
      }
      while (aggressor.remaining > 0 && !level.empty()) {
        Order* resting = level.head();
        const Qty trade_qty =
            aggressor.remaining < resting->remaining ? aggressor.remaining : resting->remaining;
        const Price trade_price = resting->price;

        level.reduce(resting, trade_qty);
        aggressor.remaining -= trade_qty;
        filled += trade_qty;

        OutboundEvent trade{};
        trade.type = EventType::Trade;
        trade.aggressor_id = aggressor.id;
        trade.resting_id = resting->id;
        trade.order_id = aggressor.id;
        trade.side = aggressor.side;
        trade.price = trade_price;
        trade.qty = trade_qty;
        trade.leaves_qty = aggressor.remaining;
        trade.ts_ns = aggressor.ts_ns;
        if (on_trade) {
          on_trade(trade);
        }

        if (resting->remaining == 0) {
          level.unlink(resting);
          index_.erase(resting->id);
          pool_.release(resting);
        }
      }
      if (level.empty()) {
        levels.erase(it);  // O(log P)
      }
    }
  };

  if (aggressor.side == Side::Buy) {
    walk(asks_, [](Price agg, Price rest) {
      return agg == 0 || agg >= rest;  // market (price 0) or limit crosses
    });
  } else {
    walk(bids_, [](Price agg, Price rest) {
      return agg == 0 || agg <= rest;
    });
  }

  refresh_tops();
  return filled;
}

bool OrderBook::cancel(OrderId id, OutboundEvent& ack) {
  // Complexity: O(1) amorti (+ O(log P) if level emptied).
  auto it = index_.find(id);
  if (it == index_.end()) {
    ack.type = EventType::Reject;
    ack.order_id = id;
    ack.reason = RejectReason::UnknownOrder;
    return false;
  }

  Order* order = it->second;
  PriceLevel* level = find_level(order->side, order->price);
  if (level == nullptr) {
    ack.type = EventType::Reject;
    ack.order_id = id;
    ack.reason = RejectReason::UnknownOrder;
    return false;
  }

  const Qty leaves = order->remaining;
  const Side side = order->side;
  const Price price = order->price;

  level->unlink(order);
  index_.erase(it);
  pool_.release(order);
  erase_level_if_empty(side, price);
  refresh_tops();

  ack.type = EventType::CancelAck;
  ack.order_id = id;
  ack.side = side;
  ack.price = price;
  ack.qty = leaves;
  ack.leaves_qty = 0;
  ack.reason = RejectReason::None;
  return true;
}

bool OrderBook::amend(OrderId id, Price new_price, Qty new_qty, OutboundEvent& ack,
                      TradeCallback on_trade) {
  auto it = index_.find(id);
  if (it == index_.end()) {
    ack.type = EventType::Reject;
    ack.order_id = id;
    ack.reason = RejectReason::UnknownOrder;
    return false;
  }

  Order* order = it->second;
  const Price old_price = order->price;
  const Side side = order->side;

  // Same price + qty decrease: in-place, keep time priority. Complexity: O(1).
  if (new_price == old_price && new_qty < order->remaining) {
    const Qty delta = order->remaining - new_qty;
    PriceLevel* level = find_level(side, old_price);
    if (level == nullptr) {
      ack.type = EventType::Reject;
      ack.order_id = id;
      ack.reason = RejectReason::UnknownOrder;
      return false;
    }
    level->reduce(order, delta);
    order->qty = new_qty;
    ack.type = EventType::AmendAck;
    ack.order_id = id;
    ack.side = side;
    ack.price = old_price;
    ack.qty = new_qty;
    ack.leaves_qty = order->remaining;
    return true;
  }

  // Price change or qty increase: cancel + reinsert (loses time priority).
  // Complexity: O(1) cancel + O(k) possible rematch + O(log P) reinsert.
  OutboundEvent cancel_ack{};
  if (!cancel(id, cancel_ack)) {
    ack = cancel_ack;
    return false;
  }

  Order* replacement = pool_.try_acquire();
  if (replacement == nullptr) {
    ack.type = EventType::Reject;
    ack.order_id = id;
    ack.reason = RejectReason::BookFull;
    return false;
  }

  *replacement = Order{};
  replacement->id = id;
  replacement->price = new_price;
  replacement->qty = new_qty;
  replacement->remaining = new_qty;
  replacement->side = side;
  replacement->type = OrderType::Limit;
  replacement->tif = TimeInForce::Day;
  replacement->ts_ns = cancel_ack.ts_ns;

  match(*replacement, std::move(on_trade));
  Qty leaves = replacement->remaining;
  if (leaves > 0) {
    if (!add_resting(replacement)) {
      pool_.release(replacement);
      ack.type = EventType::Reject;
      ack.order_id = id;
      ack.reason = RejectReason::BookFull;
      return false;
    }
  } else {
    pool_.release(replacement);
    leaves = 0;
  }

  ack.type = EventType::AmendAck;
  ack.order_id = id;
  ack.side = side;
  ack.price = new_price;
  ack.qty = new_qty;
  ack.leaves_qty = leaves;
  return true;
}

void OrderBook::refresh_tops() noexcept {
  // Complexity: O(1) — begin() of ordered map is best price.
  best_bid_ = bids_.empty() ? kInvalidPrice : bids_.begin()->first;
  best_ask_ = asks_.empty() ? kInvalidPrice : asks_.begin()->first;
}

void OrderBook::erase_level_if_empty(Side side, Price price) {
  if (side == Side::Buy) {
    auto it = bids_.find(price);
    if (it != bids_.end() && it->second.empty()) {
      bids_.erase(it);  // O(log P)
    }
  } else {
    auto it = asks_.find(price);
    if (it != asks_.end() && it->second.empty()) {
      asks_.erase(it);
    }
  }
}

PriceLevel* OrderBook::get_or_create_level(Side side, Price price) {
  if (side == Side::Buy) {
    auto [it, inserted] = bids_.try_emplace(price, price);
    (void)inserted;
    return &it->second;
  }
  auto [it, inserted] = asks_.try_emplace(price, price);
  (void)inserted;
  return &it->second;
}

PriceLevel* OrderBook::find_level(Side side, Price price) {
  if (side == Side::Buy) {
    auto it = bids_.find(price);
    return it == bids_.end() ? nullptr : &it->second;
  }
  auto it = asks_.find(price);
  return it == asks_.end() ? nullptr : &it->second;
}

}  // namespace ome
