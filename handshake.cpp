
#include "include/handshake.h"
#include <vector>
#include <thread>
bool perform_handshake(SOCKET s, const sockaddr_in& server) {

    uint8_t flags; 
    uint32_t rseq, rack; 
    uint16_t rwnd; 
    std::vector<uint8_t> rpayload;

    std::vector<uint8_t> syn = pkt::build_packet(pkt::F_SYN, 0, 0, 65535, {});
    for (int i=0;i<MAX_SYN_TRIES;i++) {
        sendto(s, reinterpret_cast<const char*>(syn.data()), syn.size(), 0, (const sockaddr*)&server, sizeof(server));
        fd_set rfds; 
        FD_ZERO(&rfds); 
        FD_SET(s,&rfds);
        timeval tv{}; 
        tv.tv_sec = SYN_RECV_TIMEOUT_SEC; 
        tv.tv_usec = 0;
        int rc = select(s+1, &rfds, nullptr, nullptr, &tv);

        if (rc > 0 && FD_ISSET(s,&rfds)) {
            uint8_t rbuf[BUFFER_SIZE]; 
            sockaddr_in from{}; 
            socklen_t fl=sizeof(from);
            ssize_t r = recvfrom(s, rbuf, sizeof(rbuf), 0, (sockaddr*)&from, &fl);
            if (r <= 0) continue;

            if (from.sin_addr.s_addr != server.sin_addr.s_addr || from.sin_port != server.sin_port) continue;
            if (!pkt::parse_packet(rbuf, (size_t)r, flags, rseq, rack, rwnd, rpayload)) continue;
            if ((flags & (pkt::F_SYN|pkt::F_ACK)) == (pkt::F_SYN|pkt::F_ACK)) {
                auto ackpkt = pkt::build_packet(pkt::F_ACK, 1, rseq, 65535, {});
                sendto(s, reinterpret_cast<const char*>(ackpkt.data()), ackpkt.size(), 0, (const sockaddr*)&server, sizeof(server));
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SYN_RETRY_WAIT_MS));
    }
    return false;
}

bool handle_handshake(SOCKET s) {
    uint8_t flags; uint32_t rseq, rack; uint16_t rwnd; std::vector<uint8_t> rpayload;

    sockaddr_in client{}; socklen_t cl=sizeof(client);

    uint8_t rbuf[BUFFER_SIZE];

    for (;;) {
        ssize_t r = recvfrom(s, rbuf, sizeof(rbuf), 0, (sockaddr*)&client, &cl);
        if (r <= 0) continue;
        if (!pkt::parse_packet(rbuf, (size_t)r, flags, rseq, rack, rwnd, rpayload)) continue;
        if (flags & pkt::F_SYN) {
            auto synack = pkt::build_packet(pkt::F_SYN|pkt::F_ACK, 0, rseq, 65535, {});

            sendto(s, reinterpret_cast<const char*>(synack.data()), synack.size(), 0, (sockaddr*)&client, cl);
            
            fd_set rfds; FD_ZERO(&rfds); FD_SET(s,&rfds);
            timeval tv{}; 
            tv.tv_sec = SYN_RECV_TIMEOUT_SEC; 
            tv.tv_usec = 0;
            int rc = select(s+1, &rfds, nullptr, nullptr, &tv);
            if (rc > 0 && FD_ISSET(s,&rfds)) {
                sockaddr_in c2{}; socklen_t c2l=sizeof(c2);
                ssize_t r2 = recvfrom(s, rbuf, sizeof(rbuf), 0, (sockaddr*)&c2, &c2l);
                if (r2 > 0 && c2.sin_addr.s_addr==client.sin_addr.s_addr && c2.sin_port==client.sin_port) {
                    if (pkt::parse_packet(rbuf, (size_t)r2, flags, rseq, rack, rwnd, rpayload) && (flags & pkt::F_ACK)) return true;
                }
            }
            return true;
        }
    }
}
