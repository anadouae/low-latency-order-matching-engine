#include "ome/matching_engine.hpp"
#include "ome/order_gateway.hpp"

#include <benchmark/benchmark.h>

#include <cstdint>

namespace {

void BM_NewCancelHotPath(benchmark::State& state) {
  ome::MatchingEngine engine(1 << 18, 1 << 16, 1 << 16);
  ome::OrderGateway gw(engine);
  std::uint64_t id = 1;

  for (auto _ : state) {
    const ome::OrderId oid = id++;
    benchmark::DoNotOptimize(gw.submit_new(oid, ome::Side::Buy, 10'000, 1));
    engine.drain(8);
    benchmark::DoNotOptimize(gw.submit_cancel(oid));
    engine.drain(8);
    ome::OutboundEvent ev{};
    while (engine.poll_event(ev)) {
    }
  }
  state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_NewCancelHotPath);

void BM_MatchCrossedBook(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    ome::MatchingEngine engine(1 << 16, 1 << 14, 1 << 14);
    ome::OrderGateway gw(engine);
    for (int i = 0; i < 128; ++i) {
      (void)gw.submit_new(static_cast<ome::OrderId>(i + 1), ome::Side::Sell, 10'000 + (i % 8), 10);
    }
    engine.drain(256);
    state.ResumeTiming();

    (void)gw.submit_new(10'000, ome::Side::Buy, 10'050, 500);
    engine.drain(8);
    ome::OutboundEvent ev{};
    while (engine.poll_event(ev)) {
    }
  }
}
BENCHMARK(BM_MatchCrossedBook);

}  // namespace

BENCHMARK_MAIN();
