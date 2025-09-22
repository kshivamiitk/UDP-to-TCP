// udp_server.cpp
// Simple UDP server: bind to a port, print incoming messages and sender address.
// Build: g++ -std=c++17 udp_server.cpp -O2 -o udp_server

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = std::stoi(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    std::cout << "[server] listening on UDP/" << port << "\n";

    while (true) {
        uint8_t buf[65536];
        sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
        uint16_t fromport = ntohs(from.sin_port);

        std::string msg(reinterpret_cast<char*>(buf), (size_t)n);
        std::cout << "[recv] " << n << " bytes from " << ipstr << ":" << fromport
                  << "  payload=\"" << msg << "\"\n";

        // (optional) echo back the message
        // ssize_t s = sendto(sock, buf, n, 0, (sockaddr*)&from, fromlen);
        // if (s < 0) perror("sendto");
    }

    close(sock);
    return 0;
}
