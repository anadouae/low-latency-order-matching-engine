#pragma once

#include "ome/order.hpp"
#include "ome/types.hpp"

namespace ome {

/// FIFO queue of orders at a single price. Doubly linked for O(1) mid-level cancel.
class PriceLevel {
 public:
  explicit PriceLevel(Price price) noexcept : price_(price) {}

  [[nodiscard]] Price price() const noexcept { return price_; }
  [[nodiscard]] Qty total_qty() const noexcept { return total_qty_; }
  [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
  [[nodiscard]] Order* head() const noexcept { return head_; }
  [[nodiscard]] Order* tail() const noexcept { return tail_; }

  /// Append to FIFO tail. Complexity: O(1).
  void push_back(Order* order) noexcept;

  /// Unlink arbitrary order (must belong to this level). Complexity: O(1).
  void unlink(Order* order) noexcept;

  /// Reduce remaining qty of an order already in this level. Complexity: O(1).
  void reduce(Order* order, Qty delta) noexcept;

 private:
  Price price_;
  Qty total_qty_{0};
  Order* head_{nullptr};
  Order* tail_{nullptr};
};

}  // namespace ome
