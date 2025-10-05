#include "../include/Packet.h"
#include "../include/checksum.h"
#include <cstring>
#include <arpa/inet.h>
#include <vector>

namespace pkt {

static constexpr size_t OFF_FLAGS = 0;
static constexpr size_t OFF_SEQ = 1;
static constexpr size_t OFF_ACK = 5;
static constexpr size_t OFF_WND = 9;
static constexpr size_t OFF_CSUM = 11;

std::vector<uint8_t> build_packet(uint8_t flags,
                                  uint32_t seq,
                                  uint32_t ack,
                                  uint16_t wnd,
                                  const std::vector<uint8_t> &payload) {
    size_t total = HEADER_SIZE + payload.size();
    std::vector<uint8_t> buf(total);
    buf[OFF_FLAGS] = flags;
    uint32_t seq_n = htonl(seq);
    memcpy(buf.data() + OFF_SEQ, &seq_n, sizeof(seq_n));
    uint32_t ack_n = htonl(ack);
    memcpy(buf.data() + OFF_ACK, &ack_n, sizeof(ack_n));
    uint16_t wnd_n = htons(wnd);
    memcpy(buf.data() + OFF_WND, &wnd_n, sizeof(wnd_n));
    buf[OFF_CSUM + 0] = 0;
    buf[OFF_CSUM + 1] = 0;
    if (!payload.empty()) memcpy(buf.data() + HEADER_SIZE, payload.data(), payload.size());
    uint16_t cs = calculate_checksum(buf.data(), buf.size());
    uint16_t cs_n = htons(cs);
    memcpy(buf.data() + OFF_CSUM, &cs_n, sizeof(cs_n));
    return buf;
}

bool parse_packet(const uint8_t *buf,
                  size_t len,
                  uint8_t &flags,
                  uint32_t &seq,
                  uint32_t &ack,
                  uint16_t &wnd,
                  std::vector<uint8_t> &payload_out) {
    if (len < HEADER_SIZE) return false;
    uint16_t stored_n;
    memcpy(&stored_n, buf + OFF_CSUM, sizeof(stored_n));
    uint16_t stored = ntohs(stored_n);
    std::vector<uint8_t> tmp(buf, buf + len);
    tmp[OFF_CSUM + 0] = 0;
    tmp[OFF_CSUM + 1] = 0;
    uint16_t calc = calculate_checksum(tmp.data(), tmp.size());
    if (calc != stored) return false;
    flags = tmp[OFF_FLAGS];
    uint32_t seq_n;
    memcpy(&seq_n, tmp.data() + OFF_SEQ, sizeof(seq_n));
    seq = ntohl(seq_n);
    uint32_t ack_n;
    memcpy(&ack_n, tmp.data() + OFF_ACK, sizeof(ack_n));
    ack = ntohl(ack_n);
    uint16_t wnd_n;
    memcpy(&wnd_n, tmp.data() + OFF_WND, sizeof(wnd_n));
    wnd = ntohs(wnd_n);
    payload_out.clear();
    if (len > HEADER_SIZE) payload_out.assign(buf + HEADER_SIZE, buf + len);
    return true;
}

} // namespace pkt
