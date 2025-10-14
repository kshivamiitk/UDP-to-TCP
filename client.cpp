
#include "include/utils.h"
#include "include/handshake.h"
#include "include/Packet.h"
#include "include/checksum.h"
#include "include/congestion.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <chrono>
struct SendNode {
    std::vector<uint8_t> bytes;
    std::chrono::steady_clock::time_point sentAt;
    bool retransmitted = false;
    int rto_ms = 1000;
};
int main(int argc, char **argv) {
    if (argc < 4) { std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <input_file>\n"; return 1; }
    const char *server_ip = argv[1];
    int server_port = std::atoi(argv[2]);
    const char *infile = argv[3];
    std::ifstream fin(infile, std::ios::binary);
    if (!fin) { std::cerr << "Failed to open input file\n"; return 1; }
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) { std::cerr << "socket failed\n"; return 1; }
    sockaddr_in server_addr{}; server_addr.sin_family = AF_INET; server_addr.sin_port = htons((uint16_t)server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) { std::cerr << "invalid ip\n"; closesocket(sock); return 1; }
    if (!perform_handshake(sock, server_addr)) { log_message("CLIENT", "Handshake failed"); closesocket(sock); return 1; }
    log_message("CLIENT", "Handshake OK");

    // handshake completed
    uint32_t send_base = 1;
    uint32_t next_seq  = 1;
    uint32_t last_cum_ack = 0;
    uint32_t inFlight = 0;
    uint16_t peer_rwnd_bytes = 65535;
    uint32_t mss_bytes = (uint32_t)CHUNK_SIZE;
    auto now_ms = [] { return std::chrono::steady_clock::now(); };
    CongestionControl cc(4, 32);
    std::map<uint32_t, SendNode> sendq;
    std::vector<uint8_t> readbuf(CHUNK_SIZE);
    bool eof = false;
    bool fin_sent = false;
    bool fin_acked = false;
    while (!fin_acked) {
        uint32_t rwndSegs = std::max<uint32_t>(peer_rwnd_bytes / mss_bytes, 1u);
        uint32_t allowance = cc.sendAllowance(inFlight, rwndSegs);
        while (!eof && allowance > 0) {
            std::streamsize got = 0;
            if (fin.good()) { 
                fin.read(reinterpret_cast<char*>(readbuf.data()), CHUNK_SIZE); 
                got = fin.gcount(); 
            }
            if (got <= 0) { 
                eof = true; 
                break;
            }
            std::vector<uint8_t> payload(readbuf.begin(), readbuf.begin() + (size_t)got);
            
            uint8_t flags = pkt::F_DATA;

            auto bytes = pkt::build_packet(flags, next_seq, 0, 65535, payload);

            ssize_t s = sendto(sock, reinterpret_cast<const char*>(bytes.data()), bytes.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

            if (s == SOCKET_ERROR) { 
                closesocket(sock); 
                return 1; 
            }
            sendq[next_seq] = { std::move(bytes), now_ms(), false, 1000 };
            ++next_seq;
            ++inFlight;
            cc.onPacketSent();
            --allowance;
        }
        if (eof && !fin_sent && cc.sendAllowance(inFlight, rwndSegs) > 0) {
            uint8_t flags = pkt::F_FIN;
            std::vector<uint8_t> payload;
            auto finpkt = pkt::build_packet(flags, next_seq, 0, 65535, payload);
            ssize_t s = sendto(sock, reinterpret_cast<const char*>(finpkt.data()), finpkt.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (s != SOCKET_ERROR) {
                sendq[next_seq] = { std::move(finpkt), now_ms(), false, 1000 };
                ++next_seq;
                ++inFlight;
                cc.onPacketSent();
                fin_sent = true;
            }
        }
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        timeval tv{}; tv.tv_sec = 0; tv.tv_usec = 10000;
        int rc = select(sock + 1, &rfds, nullptr, nullptr, &tv);
        if (rc > 0 && FD_ISSET(sock, &rfds)) {
            for (;;) {
                uint8_t rbuf[BUFFER_SIZE];
                sockaddr_in from{}; socklen_t fromlen = sizeof(from);
                ssize_t r = recvfrom(sock, rbuf, sizeof(rbuf), MSG_DONTWAIT, (struct sockaddr*)&from, &fromlen);
                if (r <= 0) break;
                if (from.sin_addr.s_addr != server_addr.sin_addr.s_addr || from.sin_port != server_addr.sin_port) continue;
                uint8_t rflags{}; uint32_t rseq{}, rack{}; uint16_t rwnd{}; std::vector<uint8_t> rpayload;
                if (!pkt::parse_packet(rbuf, (size_t)r, rflags, rseq, rack, rwnd, rpayload)) continue;
                peer_rwnd_bytes = rwnd;
                if (!(rflags & pkt::F_ACK)) continue;
                if (rack >= send_base) {
                    uint32_t prev_send_base = send_base;
                    bool partial_before = (rack + 1 < next_seq);

                    uint32_t newly = (rack + 1) - send_base;

                    for (uint32_t sseq = send_base; sseq <= rack; ++sseq) { 
                        auto it = sendq.find(sseq); 
                        if (it != sendq.end()) sendq.erase(it); 
                    }
                    send_base = rack + 1;
                    if (newly > inFlight) newly = inFlight;
                    inFlight -= newly;
                    cc.onNewAck(newly);
                    cc.resetDupAcks();
                    last_cum_ack = rack;
                    if (partial_before) {
                        auto it2 = sendq.find(send_base);
                        if (it2 != sendq.end()) {
                            auto &node = it2->second;
                            sendto(sock, reinterpret_cast<const char*>(node.bytes.data()), node.bytes.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                            node.sentAt = now_ms();
                            node.retransmitted = true;
                        }
                    }
                    if (fin_sent && rack + 1 >= next_seq) fin_acked = true;
                } else if (rack == last_cum_ack) {
                    bool hit3 = cc.onDupAck();
                    if (hit3) {
                        auto it = sendq.find(send_base);
                        if (it != sendq.end()) {
                            auto &node = it->second;
                            sendto(sock, reinterpret_cast<const char*>(node.bytes.data()), node.bytes.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                            node.sentAt = now_ms();
                            node.retransmitted = true;
                        }
                    }
                }
            }
        }
        if (!sendq.empty()) {
            auto it = sendq.find(send_base);
            if (it != sendq.end()) {
                auto &node = it->second;
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_ms() - node.sentAt).count();
                if (elapsed > node.rto_ms) {
                    sendto(sock, reinterpret_cast<const char*>(node.bytes.data()), node.bytes.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    node.sentAt = now_ms();
                    node.retransmitted = true;
                    node.rto_ms = std::min(node.rto_ms * 2, 8000);
                    cc.onTimeout();
                }
            }
        }
    }
    closesocket(sock);
    log_message("CLIENT", "Transfer complete");
    return 0;
}
