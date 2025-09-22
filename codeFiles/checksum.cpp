#include "/Users/kumarshivam/Documents/ACADEMICS SEVENTH SEMESTER/UGP COMPUTER NETWORKS/UDP_TO_TCP/structureFiles/utils.h"

uint16_t calculate_checksum(const void* data, size_t length) {
    const uint16_t* buf = reinterpret_cast<const uint16_t*>(data);
    uint32_t sum = 0;
    size_t i = 0;
    for (i = 0; i < length / 2; ++i) {
        sum += buf[i];
    }
    if (length % 2 == 1) {
        sum += *reinterpret_cast<const uint8_t*>(&buf[i]);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

bool verify_checksum(const void* data, size_t length) {
    const uint16_t* buf = reinterpret_cast<const uint16_t*>(data);
    uint32_t sum = 0;
    size_t i = 0;

    for (i = 0; i < length / 2; ++i) {
        sum += buf[i];
    }
    if (length % 2 == 1) {
        sum += *reinterpret_cast<const uint8_t*>(&buf[i]);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(sum) == 0xFFFF;
}
