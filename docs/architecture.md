# Architecture — Low-Latency Order Matching Engine

This repository implements the validated architecture for **Project 1**.

Full design (components, Mermaid diagrams, concurrency model, complexity table):

→ See the portfolio architecture document mirrored in the project store, summarized below.

## Summary

| Topic | Choice |
|---|---|
| Language | C++20 |
| Book model | Single-instrument LOB, price-time (FIFO) priority |
| Structures | `unordered_map` order index + `std::map` price levels + DLL per level |
| Concurrency | Lock-free MPSC ingress → **single-writer** matching core → SPSC egress |
| Memory | `ObjectPool<Order>` — no `new` in steady-state match loop |
| Protocol | Fixed-size binary codec (+ CLI demo façade) |
| Persistence | RAM-only (v1) |

## Critical complexity

| Op | Expected |
|---|---|
| New (rest) | O(1) amorti + O(log P) |
| New (match) | O(k) fills |
| Cancel | O(1) amorti (+ O(log P) if level emptied) |
| Amend qty↓ same px | O(1), keeps time priority |
| Amend px / qty↑ | cancel + reinsert (loses time priority) |
| Best bid/ask | O(1) cached |

Comments on critical functions in `include/ome/` and `src/` mirror this table.
