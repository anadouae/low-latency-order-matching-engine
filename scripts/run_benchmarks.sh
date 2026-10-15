#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake --preset release
cmake --build --preset release -j --target lob_hotpath_bench queue_bench
"${ROOT}/build/release/benchmarks/lob_hotpath_bench" "$@"
"${ROOT}/build/release/benchmarks/queue_bench" "$@"
