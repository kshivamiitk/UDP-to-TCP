// udp_client.cpp
// Simple UDP client: send a message to server (ip,port). Optionally wait for echo.
// Build: g++ -std=c++17 udp_client.cpp -O2 -o udp_client

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <message> [--wait]\n";
        return 1;
    }
    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    std::string message = argv[3];
    bool wait_for_reply = (argc >= 5 && std::string(argv[4]) == "--wait");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in srv;
    std::memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(server_port);
    if (inet_aton(server_ip, &srv.sin_addr) == 0) {
        std::cerr << "Invalid server IP\n";
        close(sock);
        return 1;
    }

    ssize_t sent = sendto(sock, message.data(), message.size(), 0, (sockaddr*)&srv, sizeof(srv));
    if (sent < 0) { perror("sendto"); close(sock); return 1; }
    std::cout << "[sent] " << sent << " bytes to " << server_ip << ":" << server_port << "\n";

    if (wait_for_reply) {
        uint8_t buf[65536];
        sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
        if (n < 0) { perror("recvfrom"); }
        else {
            char ipstr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
            std::string reply((char*)buf, (size_t)n);
            std::cout << "[reply] " << n << " bytes from " << ipstr << ":" << ntohs(from.sin_port)
                      << " payload=\"" << reply << "\"\n";
        }
    }

    close(sock);
    return 0;
}
