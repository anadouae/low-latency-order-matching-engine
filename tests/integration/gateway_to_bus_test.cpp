#include "ome/matching_engine.hpp"
#include "ome/order_gateway.hpp"
#include "ome/protocol/binary_codec.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace ome;

TEST(Integration, GatewayThreadedPathEmitsAckAndTrade) {
  MatchingEngine engine;
  OrderGateway gw(engine);
  engine.start();

  ASSERT_TRUE(gw.submit_new(1, Side::Sell, 200, 30));
  ASSERT_TRUE(gw.submit_new(2, Side::Buy, 200, 30));

  bool saw_trade = false;
  bool saw_ack = false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (std::chrono::steady_clock::now() < deadline && !(saw_trade && saw_ack)) {
    OutboundEvent ev{};
    if (engine.poll_event(ev)) {
      if (ev.type == EventType::Trade) {
        saw_trade = true;
      }
      if (ev.type == EventType::Ack) {
        saw_ack = true;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  engine.stop();
  EXPECT_TRUE(saw_trade);
  EXPECT_TRUE(saw_ack);
  EXPECT_EQ(engine.book().order_count(), 0u);
  EXPECT_GE(gw.accepted(), 2u);
}

TEST(Integration, BinaryCodecRoundTrip) {
  IncomingMessage in{};
  in.type = MessageType::NewOrder;
  in.order_id = 42;
  in.side = Side::Sell;
  in.order_type = OrderType::Limit;
  in.tif = TimeInForce::Day;
  in.price = 12345;
  in.qty = 99;
  in.ingress_ts_ns = 777;

  const auto bytes = BinaryCodec::encode_incoming(in);
  ASSERT_EQ(bytes.size(), BinaryCodec::kIncomingSize);
  const auto out = BinaryCodec::decode_incoming(bytes);
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->order_id, 42u);
  EXPECT_EQ(out->price, 12345);
  EXPECT_EQ(out->qty, 99);
  EXPECT_EQ(out->side, Side::Sell);
}
