rm -f server client

g++ -std=c++17 -O2 server.cpp src/handshake.cpp src/checksum.cpp -o server

g++ -std=c++17 -O2 client.cpp src/handshake.cpp src/checksum.cpp -o client

