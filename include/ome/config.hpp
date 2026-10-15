#pragma once

#include <cstddef>
#include <cstdint>

namespace ome::config {

inline constexpr std::size_t kDefaultOrderPoolCapacity = 1 << 16;  // 65_536
inline constexpr std::size_t kDefaultIngressCapacity = 1 << 14;    // 16_384 (power of 2)
inline constexpr std::size_t kDefaultEgressCapacity = 1 << 14;
inline constexpr std::int64_t kMinPriceTick = 1;
inline constexpr std::int64_t kMaxPrice = 1'000'000'000;
inline constexpr std::int64_t kMaxQty = 1'000'000'000;

}  // namespace ome::config
