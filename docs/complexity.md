# Operation Complexity

Documented on critical APIs; reproduced here for reviewers.

Assumptions: hash average O(1); DLL unlink O(1) with direct pointers; match walk O(k) where *k* = orders touched; active prices in `std::map` → O(log P).

| Operation | Time | Space | Notes |
|---|---|---|---|
| New (resting, no match) | O(1) amorti (+ O(log P)) | O(1) | Hash level + DLL push_back + index insert |
| New (aggressive match) | O(k) (+ O(log P) per emptied level) | O(1) excl. events | *k* = matched orders |
| Cancel | O(1) amorti (+ O(log P) if level removed) | O(1) | Index → unlink → aggregate |
| Amend qty↓ same price | O(1) | O(1) | In-place; time priority kept |
| Amend price or qty↑ | O(1) + New cost | O(1) | Cancel + reinsert |
| Best bid/ask | O(1) | — | Top-of-book cache |
| Total space | — | O(N + P) | *N* live orders, *P* price levels |
