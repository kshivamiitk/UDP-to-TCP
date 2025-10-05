rm -f server client

g++ -std=c++17 -O2 -Iinclude src/checksum.cpp src/Packet.cpp src/handshake.cpp server.cpp -o server
g++ -std=c++17 -O2 -Iinclude src/checksum.cpp src/Packet.cpp src/handshake.cpp client.cpp -o client
