# Low-Latency Order Matching Engine

C++20 **limit order book** with price-time (FIFO) priority, lock-free ingress, and a single-writer matching core. Built as a Staff/Principal-level portfolio project (exchange-style LOB mechanics, not a toy stub).

## Features

- Single-instrument LOB: hash order index + ordered price levels + doubly-linked FIFO per level
- **O(1)** cancel / amend-qty-down (amortized); match walk **O(k)**
- MPSC lock-free ingress → single matching thread → SPSC outbound event bus
- `ObjectPool<Order>` — no heap allocation in the steady-state match loop
- Binary protocol façade + CLI demo
- Google Test + Google Benchmark
- Docker multi-stage image + GitHub Actions (build/test/sanitize + optional benchmarks)

## Build

Requirements: CMake ≥ 3.20, Clang or GCC with C++20, Ninja, Git (FetchContent pulls GTest/Benchmark).

```bash
cmake --preset default
cmake --build --preset default -j
ctest --test-dir build/default --output-on-failure
./build/default/ome_cli --demo
```

Sanitizers:

```bash
cmake --preset sanitize && cmake --build --preset sanitize -j
ctest --test-dir build/sanitize --output-on-failure
```

Benchmarks:

```bash
./scripts/run_benchmarks.sh --benchmark_min_time=0.2s
```

Docker:

```bash
docker build -f docker/Dockerfile -t ome:local .
docker run --rm ome:local --demo
```

## Layout

```
include/ome/   public headers (types, book, engine, queues, pool, codec)
src/           implementations
tests/         unit + integration (GTest)
benchmarks/    hot-path + queue benches
docker/        multi-stage Dockerfile
.github/       CI + nightly benchmark workflow
docs/          architecture + complexity tables
```

## Complexity (critical ops)

| Op | Time |
|---|---|
| New (rest) | O(1) amorti + O(log P) |
| New (match) | O(k) |
| Cancel | O(1) amorti |
| Amend qty↓ same price | O(1), keeps time priority |
| Amend price / qty↑ | cancel + reinsert |
| Best bid/ask | O(1) |

See [`docs/complexity.md`](docs/complexity.md) and inline comments on hot-path functions.

## License

MIT — see [`LICENSE`](LICENSE).
