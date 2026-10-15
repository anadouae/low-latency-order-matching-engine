#include "ome/order.hpp"

// Order is a POD-like header-only struct; translation unit kept for ABI/stability.
namespace ome {
namespace {
[[maybe_unused]] constexpr std::size_t kOrderSize = sizeof(Order);
}
}  // namespace ome
