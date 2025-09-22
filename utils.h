#ifndef UTILS_H
#define UTILS_H

// utils.h - small portable helpers (header-only, safe to include in many .cpp)
#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

struct TCP_Head {
    bool SYN;
    bool ACK;
    uint16_t checksum;
};
inline void log_message(const std::string& agent, const std::string& msg){
    std::cout << "[" << agent << "] " << msg << std::endl;
}

inline constexpr int PORT = 8080;
inline constexpr int SERVER_PORT = 8080;
inline const char SERVER_IP[] = "127.0.0.1";
inline constexpr int BUFFER_SIZE = 1024;

#endif
