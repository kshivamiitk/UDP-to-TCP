#pragma once
#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace pkt {

// on-wire header layout (all multi-byte fields in network order)
// [0]        flags (1 byte)
// [1..4]     seq   (4 bytes)
// [5..8]     ack   (4 bytes)
// [9..10]    wnd   (2 bytes)
// [11..12]   checksum (2 bytes)
// header size = 13
inline constexpr size_t HEADER_SIZE = 13;

inline constexpr uint8_t F_SYN  = 1u << 0;
inline constexpr uint8_t F_ACK  = 1u << 1;
inline constexpr uint8_t F_FIN  = 1u << 2;
inline constexpr uint8_t F_DATA = 1u << 3;

// create a packet (header + optional payload); checksum is calculated and embedded
std::vector<uint8_t> build_packet(uint8_t flags,
                                  uint32_t seq,
                                  uint32_t ack,
                                  uint16_t wnd,
                                  const std::vector<uint8_t> &payload);

// parse and verify packet; returns true on valid packet (checksum ok and length ok)
// outputs header fields and payload
bool parse_packet(const uint8_t *buf,
                  size_t len,
                  uint8_t &flags,
                  uint32_t &seq,
                  uint32_t &ack,
                  uint16_t &wnd,
                  std::vector<uint8_t> &payload_out);

} // namespace pkt

#endif // PACKET_H
