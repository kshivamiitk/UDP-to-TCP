
#include "include/Packet.h"
#include "include/checksum.h"
#include <cstring>
#include <arpa/inet.h>
namespace pkt {
static void put_u32(std::vector<uint8_t>& b, uint32_t v) { 
    uint32_t n=htonl(v);
    const uint8_t* p=reinterpret_cast<const uint8_t*>(&n); 
    b.insert(b.end(), p, p+4); 
}
static void put_u16(std::vector<uint8_t>& b, uint16_t v) { 
    uint16_t n=htons(v); 
    const uint8_t* p=reinterpret_cast<const uint8_t*>(&n); 
    b.insert(b.end(), p, p+2); 
}
static uint32_t get_u32(const uint8_t* p) { uint32_t v; 
    std::memcpy(&v,p,4); 
    return ntohl(v);
}
static uint16_t get_u16(const uint8_t* p) { 
    uint16_t v; 
    std::memcpy(&v,p,2); 
    return ntohs(v);
}
std::vector<uint8_t> build_packet(uint8_t flags, uint32_t seq, uint32_t ack, uint16_t wnd, const std::vector<uint8_t> &payload) {
    std::vector<uint8_t> out; out.reserve(HEADER_SIZE + payload.size());
    out.push_back(flags);
    put_u32(out, seq);
    put_u32(out, ack);
    put_u16(out, wnd);
    put_u16(out, 0);
    out.insert(out.end(), payload.begin(), payload.end());
    uint16_t c = checksum16(out.data(), out.size());
    out[11] = static_cast<uint8_t>(c >> 8);
    out[12] = static_cast<uint8_t>(c & 0xFF);
    return out;
}
bool parse_packet(const uint8_t *buf, size_t len, uint8_t &flags, uint32_t &seq, uint32_t &ack, uint16_t &wnd, std::vector<uint8_t> &payload_out) {
    if (len < HEADER_SIZE) return false;
    std::vector<uint8_t> tmp(buf, buf + len);
    tmp[11] = 0; tmp[12] = 0;
    uint16_t c = checksum16(tmp.data(), tmp.size());
    if (c != (static_cast<uint16_t>(buf[11])<<8 | buf[12])) return false;
    flags = buf[0];
    seq = get_u32(buf+1);
    ack = get_u32(buf+5);
    wnd = get_u16(buf+9);
    payload_out.assign(buf+HEADER_SIZE, buf+len);
    return true;
}
}
