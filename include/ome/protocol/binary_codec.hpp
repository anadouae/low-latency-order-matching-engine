#pragma once

#include "ome/protocol/message.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace ome {

/// Minimal little-endian binary framing for demos / portfolio.
/// Layout IncomingMessage (packed intent, 40 bytes logical):
///   u8 type | u8 side | u8 order_type | u8 tif | u64 order_id | i64 price | i64 qty | u64 ts
class BinaryCodec {
 public:
  // type/side/otype/tif (4) + order_id (8) + price (8) + qty (8) + ts (8)
  static constexpr std::size_t kIncomingSize = 36;
  // type/side/reason/pad (4) + 3×u64 ids (24) + price/qty/leaves (24) + ts/seq (16)
  static constexpr std::size_t kOutboundSize = 68;

  [[nodiscard]] static std::vector<std::byte> encode_incoming(const IncomingMessage& msg);
  [[nodiscard]] static std::optional<IncomingMessage> decode_incoming(
      std::span<const std::byte> bytes);

  [[nodiscard]] static std::vector<std::byte> encode_outbound(const OutboundEvent& ev);
  [[nodiscard]] static std::optional<OutboundEvent> decode_outbound(
      std::span<const std::byte> bytes);

 private:
  template <typename T>
  static void write_le(std::vector<std::byte>& out, T value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* p = reinterpret_cast<const std::byte*>(&value);
    out.insert(out.end(), p, p + sizeof(T));
  }

  template <typename T>
  static bool read_le(std::span<const std::byte>& in, T& value) {
    if (in.size() < sizeof(T)) {
      return false;
    }
    std::memcpy(&value, in.data(), sizeof(T));
    in = in.subspan(sizeof(T));
    return true;
  }
};

}  // namespace ome
