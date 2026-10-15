#include "ome/matching_engine.hpp"

#include <utility>

namespace ome {

MatchingEngine::MatchingEngine(std::size_t pool_capacity, std::size_t ingress_capacity,
                               std::size_t egress_capacity)
    : book_(pool_capacity), ingress_(ingress_capacity), egress_(egress_capacity) {}

MatchingEngine::~MatchingEngine() { stop(); }

void MatchingEngine::start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }
  stop_requested_.store(false, std::memory_order_release);
  worker_ = std::thread([this] { run_loop(); });
}

void MatchingEngine::stop() {
  stop_requested_.store(true, std::memory_order_release);
  if (worker_.joinable()) {
    worker_.join();
  }
  running_.store(false, std::memory_order_release);
}

std::size_t MatchingEngine::drain(std::size_t max_messages) {
  std::size_t n = 0;
  IncomingMessage msg{};
  while (n < max_messages && ingress_.try_pop(msg)) {
    handle(msg);
    ++n;
  }
  return n;
}

void MatchingEngine::run_loop() {
  IncomingMessage msg{};
  while (!stop_requested_.load(std::memory_order_acquire)) {
    if (ingress_.try_pop(msg)) {
      handle(msg);
    } else {
      std::this_thread::yield();
    }
  }
  // Drain remaining after stop.
  while (ingress_.try_pop(msg)) {
    handle(msg);
  }
}

void MatchingEngine::handle(const IncomingMessage& msg) {
  const TimestampNs t0 = now_ns();

  switch (msg.type) {
    case MessageType::NewOrder: {
      telemetry_.incr_orders();
      Order aggressor{};
      aggressor.id = msg.order_id;
      aggressor.price = msg.order_type == OrderType::Market ? 0 : msg.price;
      aggressor.qty = msg.qty;
      aggressor.remaining = msg.qty;
      aggressor.side = msg.side;
      aggressor.type = msg.order_type;
      aggressor.tif = msg.tif;
      aggressor.ts_ns = msg.ingress_ts_ns != 0 ? msg.ingress_ts_ns : t0;

      if (book_.find(aggressor.id) != nullptr) {
        OutboundEvent rej{};
        rej.type = EventType::Reject;
        rej.order_id = aggressor.id;
        rej.reason = RejectReason::DuplicateOrder;
        rej.ts_ns = t0;
        publish(rej);
        telemetry_.incr_rejects();
        break;
      }

      const Qty leaves = PriceTimePriorityStrategy::apply_new(
          book_, aggressor, [this](const OutboundEvent& trade) {
            OutboundEvent t = trade;
            telemetry_.incr_trades();
            publish(std::move(t));
          });

      OutboundEvent ack{};
      ack.type = EventType::Ack;
      ack.order_id = aggressor.id;
      ack.side = aggressor.side;
      ack.price = msg.price;
      ack.qty = msg.qty;
      ack.leaves_qty = leaves;
      ack.ts_ns = t0;
      publish(ack);
      break;
    }
    case MessageType::Cancel: {
      telemetry_.incr_cancels();
      OutboundEvent ack{};
      ack.ts_ns = t0;
      if (!book_.cancel(msg.order_id, ack)) {
        telemetry_.incr_rejects();
      }
      publish(ack);
      break;
    }
    case MessageType::Amend: {
      telemetry_.incr_amends();
      OutboundEvent ack{};
      ack.ts_ns = t0;
      if (!book_.amend(
              msg.order_id, msg.price, msg.qty, ack,
              [this](const OutboundEvent& trade) {
                OutboundEvent t = trade;
                telemetry_.incr_trades();
                publish(std::move(t));
              })) {
        telemetry_.incr_rejects();
      }
      publish(ack);
      break;
    }
  }

  const TimestampNs t1 = now_ns();
  if (t1 >= t0) {
    telemetry_.record_latency_ns(t1 - t0);
  }
}

void MatchingEngine::publish(OutboundEvent ev) {
  ev.seq = ++seq_;
  // Spin briefly if egress is full — demos should size egress adequately.
  for (int i = 0; i < 1000; ++i) {
    if (egress_.try_push(ev)) {
      return;
    }
    std::this_thread::yield();
  }
  (void)egress_.try_push(ev);
}

TimestampNs MatchingEngine::now_ns() noexcept {
  using namespace std::chrono;
  return static_cast<TimestampNs>(
      duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

}  // namespace ome
