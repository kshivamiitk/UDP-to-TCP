rm -f server client

g++ -std=c++17 -O2 server.cpp codeFiles/handshake.cpp codeFiles/checksum.cpp -o server

g++ -std=c++17 -O2 client.cpp codeFiles/handshake.cpp codeFiles/checksum.cpp -o client

