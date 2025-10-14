
#pragma once
#ifndef CHECKSUM_H
#define CHECKSUM_H
#include <cstddef>
#include <cstdint>
uint16_t checksum16(const uint8_t* data, size_t len);
#endif
