
#pragma once
#ifndef PACKET_H
#define PACKET_H
#include <cstdint>
#include <cstddef>
#include <vector>
namespace pkt {
inline constexpr size_t HEADER_SIZE = 13;
inline constexpr uint8_t F_SYN  = 1u << 0;
inline constexpr uint8_t F_ACK  = 1u << 1;
inline constexpr uint8_t F_FIN  = 1u << 2;
inline constexpr uint8_t F_DATA = 1u << 3;
std::vector<uint8_t> build_packet(uint8_t flags, uint32_t seq, uint32_t ack, uint16_t wnd, const std::vector<uint8_t> &payload);
bool parse_packet(const uint8_t *buf, size_t len, uint8_t &flags, uint32_t &seq, uint32_t &ack, uint16_t &wnd, std::vector<uint8_t> &payload_out);
}
#endif
