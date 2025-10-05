#include "include/utils.h"
#include "include/handshake.h"
#include "include/Packet.h"
#include "include/checksum.h"
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <input_file>\n";
        return 1;
    }
    const char *server_ip = argv[1];
    int server_port = std::atoi(argv[2]);
    const char *infile = argv[3];
    std::ifstream fin(infile, std::ios::binary);
    if (!fin) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        return 1;
    }
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(server_port));
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        std::cerr << "invalid ip\n";
        closesocket(sock);
        return 1;
    }
    if (!perform_handshake(sock, server_addr)) {
        log_message("CLIENT", "Handshake failed");
        closesocket(sock);
        return 1;
    }
    log_message("CLIENT", "Handshake OK");
    uint32_t seq = 1;
    bool last = false;
    std::vector<uint8_t> readbuf(CHUNK_SIZE);
    while (!last) {
        fin.read(reinterpret_cast<char*>(readbuf.data()), CHUNK_SIZE);
        std::streamsize got = fin.gcount();
        if (got < static_cast<std::streamsize>(CHUNK_SIZE)) last = true;
        std::vector<uint8_t> payload(readbuf.begin(), readbuf.begin() + static_cast<size_t>(got));
        uint8_t flags = pkt::F_DATA;
        if (last) flags = pkt::F_FIN | pkt::F_DATA;
        std::vector<uint8_t> pkt = pkt::build_packet(flags, seq, 0, 65535, payload);
        bool acked = false;
        for (int attempt = 0; attempt < MAX_DATA_RETRIES && !acked; ++attempt) {
            ssize_t s = sendto(sock, reinterpret_cast<const char*>(pkt.data()), pkt.size(), 0,
                               (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (s == SOCKET_ERROR) {
                closesocket(sock);
                return 1;
            }
            int waited = 0;
            while (waited < DATA_ACK_TIMEOUT_MS) {
                uint8_t rbuf[BUFFER_SIZE];
                sockaddr_in from{};
                socklen_t fromlen = sizeof(from);
                ssize_t r = recvfrom(sock, rbuf, sizeof(rbuf), 0, (struct sockaddr*)&from, &fromlen);
                if (r > 0) {
                    uint8_t rflags;
                    uint32_t rseq, rack;
                    uint16_t rwnd;
                    std::vector<uint8_t> rpayload;
                    if (!pkt::parse_packet(rbuf, (size_t)r, rflags, rseq, rack, rwnd, rpayload)) {
                        continue;
                    }
                    if ((rflags & pkt::F_ACK) && rseq == seq) {
                        acked = true;
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(DATA_ACK_POLL_MS));
                waited += DATA_ACK_POLL_MS;
            }
        }
        if (!acked) {
            log_message("CLIENT", "Failed to get ACK after retries, aborting");
            closesocket(sock);
            return 1;
        }
        seq++;
    }
    closesocket(sock);
    log_message("CLIENT", "Transfer complete");
    return 0;
}
