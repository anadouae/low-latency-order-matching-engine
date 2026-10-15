#include "ome/protocol/binary_codec.hpp"

#include <type_traits>

namespace ome {

std::vector<std::byte> BinaryCodec::encode_incoming(const IncomingMessage& msg) {
  std::vector<std::byte> out;
  out.reserve(kIncomingSize);
  write_le(out, static_cast<std::uint8_t>(msg.type));
  write_le(out, static_cast<std::uint8_t>(msg.side));
  write_le(out, static_cast<std::uint8_t>(msg.order_type));
  write_le(out, static_cast<std::uint8_t>(msg.tif));
  write_le(out, msg.order_id);
  write_le(out, msg.price);
  write_le(out, msg.qty);
  write_le(out, msg.ingress_ts_ns);
  return out;
}

std::optional<IncomingMessage> BinaryCodec::decode_incoming(std::span<const std::byte> bytes) {
  if (bytes.size() < kIncomingSize) {
    return std::nullopt;
  }
  IncomingMessage msg{};
  std::uint8_t type = 0, side = 0, otype = 0, tif = 0;
  auto in = bytes;
  if (!read_le(in, type) || !read_le(in, side) || !read_le(in, otype) || !read_le(in, tif) ||
      !read_le(in, msg.order_id) || !read_le(in, msg.price) || !read_le(in, msg.qty) ||
      !read_le(in, msg.ingress_ts_ns)) {
    return std::nullopt;
  }
  msg.type = static_cast<MessageType>(type);
  msg.side = static_cast<Side>(side);
  msg.order_type = static_cast<OrderType>(otype);
  msg.tif = static_cast<TimeInForce>(tif);
  msg.client_order_id = msg.order_id;
  return msg;
}

std::vector<std::byte> BinaryCodec::encode_outbound(const OutboundEvent& ev) {
  std::vector<std::byte> out;
  out.reserve(kOutboundSize);
  write_le(out, static_cast<std::uint8_t>(ev.type));
  write_le(out, static_cast<std::uint8_t>(ev.side));
  write_le(out, static_cast<std::uint8_t>(ev.reason));
  write_le(out, static_cast<std::uint8_t>(0));  // pad
  write_le(out, ev.order_id);
  write_le(out, ev.aggressor_id);
  write_le(out, ev.resting_id);
  write_le(out, ev.price);
  write_le(out, ev.qty);
  write_le(out, ev.leaves_qty);
  write_le(out, ev.ts_ns);
  write_le(out, ev.seq);
  return out;
}

std::optional<OutboundEvent> BinaryCodec::decode_outbound(std::span<const std::byte> bytes) {
  if (bytes.size() < kOutboundSize) {
    return std::nullopt;
  }
  OutboundEvent ev{};
  std::uint8_t type = 0, side = 0, reason = 0, pad = 0;
  auto in = bytes;
  if (!read_le(in, type) || !read_le(in, side) || !read_le(in, reason) || !read_le(in, pad) ||
      !read_le(in, ev.order_id) || !read_le(in, ev.aggressor_id) || !read_le(in, ev.resting_id) ||
      !read_le(in, ev.price) || !read_le(in, ev.qty) || !read_le(in, ev.leaves_qty) ||
      !read_le(in, ev.ts_ns) || !read_le(in, ev.seq)) {
    return std::nullopt;
  }
  ev.type = static_cast<EventType>(type);
  ev.side = static_cast<Side>(side);
  ev.reason = static_cast<RejectReason>(reason);
  return ev;
}

}  // namespace ome
