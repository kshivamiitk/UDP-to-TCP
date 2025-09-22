#pragma once
#include <cstdint>
#include <cstddef>

uint16_t calculate_checksum(const void* data, size_t length);
bool verify_checksum(const void* data, size_t length);
