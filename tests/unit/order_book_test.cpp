#include "ome/order_book.hpp"
#include "sample_orders.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace ome;

TEST(OrderBook, RestingAddMaintainsBestBidAsk) {
  OrderBook book(1024);

  Order* bid = book.pool().acquire();
  *bid = Order{};
  bid->id = 1;
  bid->side = Side::Buy;
  bid->price = 100;
  bid->qty = 10;
  bid->remaining = 10;
  ASSERT_TRUE(book.add_resting(bid));

  Order* ask = book.pool().acquire();
  *ask = Order{};
  ask->id = 2;
  ask->side = Side::Sell;
  ask->price = 105;
  ask->qty = 7;
  ask->remaining = 7;
  ASSERT_TRUE(book.add_resting(ask));

  EXPECT_EQ(book.best_bid(), 100);
  EXPECT_EQ(book.best_ask(), 105);
  EXPECT_EQ(book.order_count(), 2u);
}

TEST(OrderBook, PriceTimeFifoWithinLevel) {
  OrderBook book(1024);
  std::vector<OrderId> fill_order;

  auto rest = [&](OrderId id, Qty qty) {
    Order* o = book.pool().acquire();
    *o = Order{};
    o->id = id;
    o->side = Side::Sell;
    o->price = 100;
    o->qty = qty;
    o->remaining = qty;
    ASSERT_TRUE(book.add_resting(o));
  };
  rest(1, 10);
  rest(2, 10);
  rest(3, 10);

  Order agg{};
  agg.id = 99;
  agg.side = Side::Buy;
  agg.price = 100;
  agg.qty = 25;
  agg.remaining = 25;

  book.match(agg, [&](const OutboundEvent& t) { fill_order.push_back(t.resting_id); });

  ASSERT_EQ(fill_order.size(), 3u);
  EXPECT_EQ(fill_order[0], 1u);
  EXPECT_EQ(fill_order[1], 2u);
  EXPECT_EQ(fill_order[2], 3u);
  EXPECT_EQ(agg.remaining, 0);
  // 25 filled against 10+10+10 → order 3 left with 5.
  ASSERT_NE(book.find(3), nullptr);
  EXPECT_EQ(book.find(3)->remaining, 5);
  EXPECT_EQ(book.order_count(), 1u);
}

TEST(OrderBook, PartialFillLeavesResting) {
  OrderBook book(1024);

  Order* ask = book.pool().acquire();
  *ask = Order{};
  ask->id = 1;
  ask->side = Side::Sell;
  ask->price = 50;
  ask->qty = 100;
  ask->remaining = 100;
  ASSERT_TRUE(book.add_resting(ask));

  Order agg{};
  agg.id = 2;
  agg.side = Side::Buy;
  agg.price = 50;
  agg.qty = 40;
  agg.remaining = 40;

  const Qty filled = book.match(agg, [](const OutboundEvent&) {});
  EXPECT_EQ(filled, 40);
  EXPECT_EQ(agg.remaining, 0);
  ASSERT_NE(book.find(1), nullptr);
  EXPECT_EQ(book.find(1)->remaining, 60);
}
