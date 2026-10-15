#include "ome/matching_engine.hpp"
#include "sample_orders.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace ome;
using ome::fixtures::make_amend;
using ome::fixtures::make_cancel;
using ome::fixtures::make_new;

TEST(CancelAmend, CancelMidLevelPreservesFifoNeighbors) {
  MatchingEngine engine;
  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Sell, 100, 10)));
  ASSERT_TRUE(engine.enqueue(make_new(2, Side::Sell, 100, 10)));
  ASSERT_TRUE(engine.enqueue(make_new(3, Side::Sell, 100, 10)));
  ASSERT_TRUE(engine.enqueue(make_cancel(2)));
  engine.drain();

  EXPECT_EQ(engine.book().find(2), nullptr);
  ASSERT_NE(engine.book().find(1), nullptr);
  ASSERT_NE(engine.book().find(3), nullptr);

  // Next aggressor should hit id=1 then id=3 (FIFO after mid cancel).
  ASSERT_TRUE(engine.enqueue(make_new(9, Side::Buy, 100, 15)));
  engine.drain();

  std::vector<OrderId> resting_hit;
  OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    if (ev.type == EventType::Trade) {
      resting_hit.push_back(ev.resting_id);
    }
  }
  ASSERT_GE(resting_hit.size(), 2u);
  EXPECT_EQ(resting_hit[0], 1u);
  EXPECT_EQ(resting_hit[1], 3u);
}

TEST(CancelAmend, AmendQtyDownKeepsPriority) {
  MatchingEngine engine;
  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Sell, 100, 50)));
  ASSERT_TRUE(engine.enqueue(make_new(2, Side::Sell, 100, 50)));
  ASSERT_TRUE(engine.enqueue(make_amend(1, 100, 20)));
  engine.drain();

  ASSERT_NE(engine.book().find(1), nullptr);
  EXPECT_EQ(engine.book().find(1)->remaining, 20);

  ASSERT_TRUE(engine.enqueue(make_new(9, Side::Buy, 100, 20)));
  engine.drain();

  OrderId first_resting = 0;
  OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    if (ev.type == EventType::Trade && first_resting == 0) {
      first_resting = ev.resting_id;
    }
  }
  EXPECT_EQ(first_resting, 1u);
}

TEST(CancelAmend, AmendPriceLosesTimePriority) {
  MatchingEngine engine;
  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Sell, 100, 10)));
  ASSERT_TRUE(engine.enqueue(make_new(2, Side::Sell, 100, 10)));
  // Move order 1 to a new price then back — treated as cancel+reinsert → behind 2 at 100.
  ASSERT_TRUE(engine.enqueue(make_amend(1, 101, 10)));
  ASSERT_TRUE(engine.enqueue(make_amend(1, 100, 10)));
  engine.drain();

  ASSERT_TRUE(engine.enqueue(make_new(9, Side::Buy, 100, 10)));
  engine.drain();

  OrderId first_resting = 0;
  OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    if (ev.type == EventType::Trade && first_resting == 0) {
      first_resting = ev.resting_id;
    }
  }
  EXPECT_EQ(first_resting, 2u);
}
