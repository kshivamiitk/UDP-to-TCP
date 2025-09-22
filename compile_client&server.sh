rm -f server client

g++ -std=c++17 -O2 server.cpp handshake.cpp checksum.cpp -o server

g++ -std=c++17 -O2 client.cpp handshake.cpp checksum.cpp -o client

