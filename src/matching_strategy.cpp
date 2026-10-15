#include "ome/matching_strategy.hpp"

namespace ome {

Qty PriceTimePriorityStrategy::apply_new(OrderBook& book, Order& order,
                                         OrderBook::TradeCallback on_trade) {
  book.match(order, on_trade);

  if (order.remaining <= 0) {
    return 0;
  }

  if (order.type == OrderType::Market || order.tif == TimeInForce::Ioc) {
    // No resting for market / IOC — leftover cancelled implicitly.
    return 0;
  }

  Order* resting = book.pool().try_acquire();
  if (resting == nullptr) {
    order.remaining = 0;
    return 0;
  }

  *resting = order;
  resting->prev = nullptr;
  resting->next = nullptr;
  if (!book.add_resting(resting)) {
    book.pool().release(resting);
    return 0;
  }
  return resting->remaining;
}

}  // namespace ome
