#include "ome/matching_engine.hpp"
#include "sample_orders.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace ome;
using ome::fixtures::make_new;

TEST(MatchingEngine, AggressiveLimitGeneratesTradesAndAck) {
  MatchingEngine engine(2048, 1024, 2048);

  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Sell, 100, 50)));
  ASSERT_TRUE(engine.enqueue(make_new(2, Side::Sell, 101, 50)));
  ASSERT_TRUE(engine.enqueue(make_new(10, Side::Buy, 101, 75)));
  EXPECT_EQ(engine.drain(), 3u);

  std::vector<OutboundEvent> events;
  OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    events.push_back(ev);
  }

  int trades = 0;
  int acks = 0;
  for (const auto& e : events) {
    if (e.type == EventType::Trade) {
      ++trades;
    }
    if (e.type == EventType::Ack && e.order_id == 10) {
      ++acks;
      EXPECT_EQ(e.leaves_qty, 0);
    }
  }
  EXPECT_GE(trades, 2);
  EXPECT_EQ(acks, 1);
  EXPECT_EQ(engine.book().best_ask(), 101);
  EXPECT_NE(engine.book().find(2), nullptr);
  EXPECT_EQ(engine.book().find(2)->remaining, 25);
}

TEST(MatchingEngine, DuplicateOrderRejected) {
  MatchingEngine engine;
  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Buy, 10, 1)));
  ASSERT_TRUE(engine.enqueue(make_new(1, Side::Buy, 11, 1)));
  engine.drain();

  bool saw_reject = false;
  OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    if (ev.type == EventType::Reject && ev.reason == RejectReason::DuplicateOrder) {
      saw_reject = true;
    }
  }
  EXPECT_TRUE(saw_reject);
}
