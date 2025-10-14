
#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>
#include <string>
#include <cstdint>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
inline constexpr int PORT = 8080;
inline constexpr int SERVER_PORT = 8080;
inline const char SERVER_IP[] = "127.0.0.1";
inline constexpr int BUFFER_SIZE = 4096;
inline constexpr int MAX_SYN_TRIES = 5;
inline constexpr int SYN_RETRY_WAIT_MS = 250;
inline constexpr int SYN_RECV_TIMEOUT_SEC = 1;
inline constexpr size_t CHUNK_SIZE = 1000;
inline constexpr int MAX_DATA_RETRIES = 8;
inline constexpr int DATA_ACK_TIMEOUT_MS = 1000;
inline constexpr int DATA_ACK_POLL_MS = 50;
inline void log_message(const std::string &agent, const std::string &msg) { std::cout << "[" << agent << "] " << msg << std::endl; }
#endif
