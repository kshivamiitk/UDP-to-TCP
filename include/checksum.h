#pragma once
#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <cstddef>
#include <cstdint>

// compute 16-bit ones' complement checksum over `data`
// `data` is a byte buffer of length `length`.
uint16_t calculate_checksum(const void *data, size_t length);

// verify that a buffer containing a 16-bit checksum (somewhere in the buffer)
// yields a valid ones' complement total (i.e., sum == 0xFFFF).
// This is a generic byte-wise verifier: it returns true when the ones'-complement
// sum of all 16-bit words equals 0xFFFF.
bool verify_checksum_buffer(const void *data, size_t length);

#endif // CHECKSUM_H
