#include "ome/price_level.hpp"

namespace ome {

void PriceLevel::push_back(Order* order) noexcept {
  order->prev = tail_;
  order->next = nullptr;
  if (tail_ != nullptr) {
    tail_->next = order;
  } else {
    head_ = order;
  }
  tail_ = order;
  total_qty_ += order->remaining;
}

void PriceLevel::unlink(Order* order) noexcept {
  total_qty_ -= order->remaining;
  if (order->prev != nullptr) {
    order->prev->next = order->next;
  } else {
    head_ = order->next;
  }
  if (order->next != nullptr) {
    order->next->prev = order->prev;
  } else {
    tail_ = order->prev;
  }
  order->prev = nullptr;
  order->next = nullptr;
}

void PriceLevel::reduce(Order* order, Qty delta) noexcept {
  order->remaining -= delta;
  total_qty_ -= delta;
}

}  // namespace ome
