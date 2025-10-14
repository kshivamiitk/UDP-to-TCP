
#include "include/utils.h"
#include "include/handshake.h"
#include "include/Packet.h"
#include "include/checksum.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
struct OOOEntry { 
    std::vector<uint8_t> payload; 
    bool fin=false;
};
int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <output_file> [port]\n"; return 1; }
    const char *outfile = argv[1];
    int port = (argc >= 3) ? std::atoi(argv[2]) : PORT;
    
    std::ofstream fout(outfile, std::ios::binary | std::ios::trunc);
    
    if (!fout) { 
        std::cerr << "failed open out\n"; 
        return 1; 
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == INVALID_SOCKET) { 
        std::cerr << "socket failed\n"; 
        return 1; 
    }
    sockaddr_in svr{}; 
    svr.sin_family = AF_INET; 
    svr.sin_addr.s_addr = INADDR_ANY; 
    svr.sin_port = htons((uint16_t)port);

    if (bind(sock, (struct sockaddr*)&svr, sizeof(svr)) == SOCKET_ERROR) { 
        std::cerr << "bind failed\n"; 
        closesocket(sock); 
        return 1; 
    }
    log_message("SERVER", "Server listening on port " + std::to_string(port));
    
    for (;;) {
        log_message("SERVER", "Waiting for handshake...");
        if (!handle_handshake(sock)) { log_message("SERVER", "Handshake failed - continue"); continue; }
        log_message("SERVER", "Handshake complete");
        // handshake completed
        uint32_t expected = 1;
        bool finished = false;
        std::map<uint32_t, OOOEntry> ooo;
        while (!finished) {
            uint8_t rbuf[BUFFER_SIZE];

            sockaddr_in client{}; socklen_t clientlen = sizeof(client);

            ssize_t r = recvfrom(sock, rbuf, sizeof(rbuf), 0, (struct sockaddr*)&client, &clientlen);
            
            if (r <= 0) continue;

            uint8_t flags; uint32_t seq, ack; uint16_t wnd; std::vector<uint8_t> payload;
            
            if (!pkt::parse_packet(rbuf, (size_t)r, flags, seq, ack, wnd, payload)) continue;
            
            if (seq == expected) {
                if (!payload.empty()) { 
                    fout.write(reinterpret_cast<const char*>(payload.data()), payload.size()); 
                    fout.flush();
                }
                bool fin_here = (flags & pkt::F_FIN);
                ++expected;
                while (true) {
                    auto it = ooo.find(expected);
                    if (it == ooo.end()) break;
                    if (!it->second.payload.empty()) { 
                        fout.write(reinterpret_cast<const char*>(it->second.payload.data()), it->second.payload.size()); 
                        fout.flush(); 
                    }
                    bool fin_buf = it->second.fin;
                    ooo.erase(it);
                    if (fin_buf) fin_here = true;
                    ++expected;
                }
                auto ackpkt = pkt::build_packet(pkt::F_ACK, 0, expected - 1, 65535, {});
                sendto(sock, reinterpret_cast<const char*>(ackpkt.data()), ackpkt.size(), 0, (struct sockaddr*)&client, clientlen);
                if (fin_here) finished = true;
            } else if (seq > expected) {
                auto &entry = ooo[seq];
                if (entry.payload.empty() && !payload.empty()) entry.payload = payload;
                if (flags & pkt::F_FIN) entry.fin = true;
                auto ackpkt = pkt::build_packet(pkt::F_ACK, 0, expected - 1, 65535, {});
                sendto(sock, reinterpret_cast<const char*>(ackpkt.data()), ackpkt.size(), 0, (struct sockaddr*)&client, clientlen);
            } else {
                auto ackpkt = pkt::build_packet(pkt::F_ACK, 0, expected - 1, 65535, {});
                sendto(sock, reinterpret_cast<const char*>(ackpkt.data()), ackpkt.size(), 0, (struct sockaddr*)&client, clientlen);
            }
        }
        log_message("SERVER", "Transfer complete - awaiting next client");
    }
    return 0;
}
