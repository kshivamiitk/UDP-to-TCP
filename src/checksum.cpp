#include "../include/checksum.h"
#include <cstdint>
#include <cstddef>

uint16_t calculate_checksum(const void *data, size_t length) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(data);
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < length) {
        uint16_t w = (uint16_t(bytes[i]) << 8) | uint16_t(bytes[i + 1]);
        sum += w;
        if (sum & 0x10000) sum = (sum & 0xffff) + 1;
        i += 2;
    }
    if (i < length) {
        uint16_t w = (uint16_t(bytes[i]) << 8);
        sum += w;
        if (sum & 0x10000) sum = (sum & 0xffff) + 1;
    }
    return static_cast<uint16_t>(~(sum & 0xffff));
}

bool verify_checksum_buffer(const void *data, size_t length) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(data);
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < length) {
        uint16_t w = (uint16_t(bytes[i]) << 8) | uint16_t(bytes[i + 1]);
        sum += w;
        if (sum & 0x10000) sum = (sum & 0xffff) + 1;
        i += 2;
    }
    if (i < length) {
        uint16_t w = (uint16_t(bytes[i]) << 8);
        sum += w;
        if (sum & 0x10000) sum = (sum & 0xffff) + 1;
    }
    return static_cast<uint16_t>(sum & 0xffff) == 0xffff;
}
