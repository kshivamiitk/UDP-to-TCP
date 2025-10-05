#pragma once
#ifndef UTILS_H
#define UTILS_H
#include<unistd.h>

#include <string>
#include <cstdint>
#include <chrono>
#include <arpa/inet.h>

#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

// network defaults
inline constexpr int PORT = 8080;
inline constexpr int SERVER_PORT = 8080;
inline const char SERVER_IP[] = "127.0.0.1";

// buffer sizes
inline constexpr int BUFFER_SIZE = 4096;

// handshake & retry parameters
inline constexpr int MAX_SYN_TRIES = 5;
inline constexpr int SYN_RETRY_WAIT_MS = 250;
inline constexpr int SYN_RECV_TIMEOUT_SEC = 1;

// data-transfer parameters
inline constexpr size_t CHUNK_SIZE = 1000;
inline constexpr int MAX_DATA_RETRIES = 8;
inline constexpr int DATA_ACK_TIMEOUT_MS = 1000;
inline constexpr int DATA_ACK_POLL_MS = 50;

// logging helper
inline void log_message(const std::string &agent, const std::string &msg) {
    std::cout << "[" << agent << "] " << msg << std::endl;
}

#endif // UTILS_H
