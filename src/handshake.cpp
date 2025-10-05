#include "../include/handshake.h"
#include "../include/Packet.h"
#include "../include/checksum.h"
#include "../include/utils.h"
#include <vector>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <cstring>

bool perform_handshake(SOCKET client_socket, sockaddr_in &server_address) {
    using namespace std::chrono;
    const int max_tries = MAX_SYN_TRIES;
    const int wait_ms = SYN_RETRY_WAIT_MS;
    const uint16_t default_wnd = 65535;
    uint8_t syn_flags = pkt::F_SYN;
    std::vector<uint8_t> syn_pkt = pkt::build_packet(syn_flags, 0, 0, default_wnd, {});
    timeval tv{SYN_RECV_TIMEOUT_SEC, 0};
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    for (int attempt = 1; attempt <= max_tries; ++attempt) {
        log_message("CLIENT", std::string("STEP 1: sending SYN (attempt ") + std::to_string(attempt) + ")");
        ssize_t sent = sendto(client_socket, reinterpret_cast<const char*>(syn_pkt.data()), syn_pkt.size(), 0,
                              (struct sockaddr*)&server_address, sizeof(server_address));
        if (sent == SOCKET_ERROR) return false;
        uint8_t rbuf[BUFFER_SIZE];
        sockaddr_in from{};
        socklen_t fromlen = sizeof(from);
        ssize_t r = recvfrom(client_socket, rbuf, BUFFER_SIZE, 0, (struct sockaddr*)&from, &fromlen);
        if (r <= 0) {
            std::this_thread::sleep_for(milliseconds(wait_ms));
            continue;
        }
        uint8_t flags;
        uint32_t seq, ack;
        uint16_t wnd;
        std::vector<uint8_t> payload;
        bool ok = pkt::parse_packet(rbuf, (size_t)r, flags, seq, ack, wnd, payload);
        if (!ok) continue;
        if ((flags & pkt::F_SYN) && (flags & pkt::F_ACK)) {
            log_message("CLIENT", "STEP 2: Received valid SYN-ACK from server.");
            uint8_t ack_flags = pkt::F_ACK;
            std::vector<uint8_t> ack_pkt = pkt::build_packet(ack_flags, 0, seq + 1, default_wnd, {});
            ssize_t s2 = sendto(client_socket, reinterpret_cast<const char*>(ack_pkt.data()), ack_pkt.size(), 0,
                                 (struct sockaddr*)&server_address, sizeof(server_address));
            if (s2 == SOCKET_ERROR) return false;
            log_message("CLIENT", "STEP 3: Sent final ACK. Handshake complete.");
            return true;
        }
    }
    log_message("CLIENT", "handshake failed (no SYN-ACK)");
    return false;
}

bool handle_handshake(SOCKET server_socket) {
    uint8_t rbuf[BUFFER_SIZE];
    sockaddr_in client{};
    socklen_t clientlen = sizeof(client);
    ssize_t r = recvfrom(server_socket, rbuf, BUFFER_SIZE, 0, (struct sockaddr*)&client, &clientlen);
    if (r <= 0) {
        log_message("SERVER", "recvfrom() error while waiting for SYN");
        return false;
    }
    uint8_t flags;
    uint32_t seq, ack;
    uint16_t wnd;
    std::vector<uint8_t> payload;
    bool ok = pkt::parse_packet(rbuf, (size_t)r, flags, seq, ack, wnd, payload);
    if (!ok) {
        log_message("SERVER", "Invalid checksum on received SYN. Discarding.");
        return false;
    }
    if (!(flags & pkt::F_SYN) || (flags & pkt::F_ACK)) {
        log_message("SERVER", "Received non-SYN or badly-flagged packet. Ignoring.");
        return false;
    }
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);
    log_message("SERVER", std::string("STEP 1: Received valid SYN from ") + std::string(client_ip));
    uint8_t synack_flags = pkt::F_SYN | pkt::F_ACK;
    std::vector<uint8_t> synack_pkt = pkt::build_packet(synack_flags, 0, seq + 1, wnd, {});
    ssize_t s = sendto(server_socket, reinterpret_cast<const char*>(synack_pkt.data()), synack_pkt.size(), 0,
                       (struct sockaddr*)&client, clientlen);
    if (s == SOCKET_ERROR) {
        log_message("SERVER", "sendto() failed for SYN-ACK");
        return false;
    }
    log_message("SERVER", "STEP 2: Sent SYN-ACK to client");
    ssize_t r2 = recvfrom(server_socket, rbuf, BUFFER_SIZE, 0, (struct sockaddr*)&client, &clientlen);
    if (r2 <= 0) {
        log_message("SERVER", "recvfrom() error while waiting for final ACK");
        return false;
    }
    ok = pkt::parse_packet(rbuf, (size_t)r2, flags, seq, ack, wnd, payload);
    if (!ok) {
        log_message("SERVER", "Invalid checksum on received ACK. Discarding.");
        return false;
    }
    if (!(flags & pkt::F_SYN) && (flags & pkt::F_ACK)) {
        log_message("SERVER", "STEP 3: Received valid ACK from client. Handshake complete.");
        return true;
    } else {
        log_message("SERVER", "SERVER: final packet not ACK as expected");
        return false;
    }
}
