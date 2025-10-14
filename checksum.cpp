
#include "include/checksum.h"
#include <cstdint>
uint16_t checksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    const uint16_t* p = reinterpret_cast<const uint16_t*>(data);
    while (len > 1) { 
        sum += *p++; 
        len -= 2; 
    }
    if (len) sum += static_cast<uint16_t>(*(reinterpret_cast<const uint8_t*>(p)));

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

    return static_cast<uint16_t>(~sum);
}
