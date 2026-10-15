#include "ome/matching_engine.hpp"
#include "ome/order_gateway.hpp"
#include "ome/protocol/binary_codec.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

void print_event(const ome::OutboundEvent& ev) {
  using ome::EventType;
  switch (ev.type) {
    case EventType::Ack:
      std::cout << "ACK id=" << ev.order_id << " leaves=" << ev.leaves_qty << " seq=" << ev.seq
                << '\n';
      break;
    case EventType::Trade:
      std::cout << "TRADE agg=" << ev.aggressor_id << " rest=" << ev.resting_id
                << " px=" << ev.price << " qty=" << ev.qty << " seq=" << ev.seq << '\n';
      break;
    case EventType::CancelAck:
      std::cout << "CANCEL_ACK id=" << ev.order_id << " seq=" << ev.seq << '\n';
      break;
    case EventType::AmendAck:
      std::cout << "AMEND_ACK id=" << ev.order_id << " px=" << ev.price << " qty=" << ev.qty
                << " leaves=" << ev.leaves_qty << " seq=" << ev.seq << '\n';
      break;
    case EventType::Reject:
      std::cout << "REJECT id=" << ev.order_id
                << " reason=" << static_cast<int>(ev.reason) << " seq=" << ev.seq << '\n';
      break;
  }
}

void run_demo() {
  ome::MatchingEngine engine;
  ome::OrderGateway gateway(engine);
  engine.start();

  // Seed resting liquidity on the ask side.
  (void)gateway.submit_new(1, ome::Side::Sell, 10050, 100);
  (void)gateway.submit_new(2, ome::Side::Sell, 10050, 50);
  (void)gateway.submit_new(3, ome::Side::Sell, 10100, 200);

  // Aggressive buy that crosses both levels partially.
  (void)gateway.submit_new(10, ome::Side::Buy, 10100, 180);

  // Cancel remaining resting at 10100 if any leftover of id=3.
  (void)gateway.submit_cancel(3);

  // Place bid and amend qty down.
  (void)gateway.submit_new(20, ome::Side::Buy, 9950, 75);
  (void)gateway.submit_amend(20, 9950, 40);

  // Let the matching thread drain.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  ome::OutboundEvent ev{};
  while (engine.poll_event(ev)) {
    print_event(ev);
  }

  const auto& tel = engine.telemetry();
  auto px = [](ome::Price p) -> std::string {
    return p == ome::kInvalidPrice ? "none" : std::to_string(p);
  };
  std::cout << "\n--- telemetry ---\n"
            << "orders=" << tel.orders() << " cancels=" << tel.cancels()
            << " amends=" << tel.amends() << " trades=" << tel.trades()
            << " rejects=" << tel.rejects() << '\n'
            << "latency_samples=" << tel.latency_samples()
            << " sum_ns=" << tel.latency_sum_ns() << '\n'
            << "best_bid=" << px(engine.book().best_bid())
            << " best_ask=" << px(engine.book().best_ask())
            << " open_orders=" << engine.book().order_count() << '\n';

  // Round-trip a binary frame as a façade smoke check.
  ome::IncomingMessage wire{};
  wire.type = ome::MessageType::NewOrder;
  wire.order_id = 99;
  wire.side = ome::Side::Buy;
  wire.price = 9900;
  wire.qty = 10;
  const auto encoded = ome::BinaryCodec::encode_incoming(wire);
  const auto decoded = ome::BinaryCodec::decode_incoming(encoded);
  std::cout << "binary_codec_ok=" << (decoded && decoded->order_id == 99) << '\n';

  engine.stop();
}

void print_usage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " [--demo]\n"
            << "  --demo   run a single-instrument price-time matching demo (default)\n";
}

}  // namespace

int main(int argc, char** argv) {
  bool demo = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
    if (arg == "--demo") {
      demo = true;
      continue;
    }
    std::cerr << "Unknown argument: " << arg << '\n';
    print_usage(argv[0]);
    return 2;
  }

  if (demo) {
    run_demo();
  }
  return 0;
}
