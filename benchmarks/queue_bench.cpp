#include "ome/concurrency/mpsc_queue.hpp"
#include "ome/concurrency/spsc_queue.hpp"

#include <benchmark/benchmark.h>

namespace {

void BM_SpscRoundTrip(benchmark::State& state) {
  ome::SpscQueue<std::uint64_t> q(4096);
  std::uint64_t seq = 0;
  for (auto _ : state) {
    const std::uint64_t v = ++seq;
    while (!q.try_push(v)) {
    }
    std::uint64_t out = 0;
    while (!q.try_pop(out)) {
    }
    benchmark::DoNotOptimize(out);
  }
}
BENCHMARK(BM_SpscRoundTrip);

void BM_MpscSingleProducer(benchmark::State& state) {
  ome::MpscQueue<std::uint64_t> q(4096);
  std::uint64_t seq = 0;
  for (auto _ : state) {
    const std::uint64_t v = ++seq;
    while (!q.try_push(v)) {
    }
    std::uint64_t out = 0;
    while (!q.try_pop(out)) {
    }
    benchmark::DoNotOptimize(out);
  }
}
BENCHMARK(BM_MpscSingleProducer);

}  // namespace

BENCHMARK_MAIN();
