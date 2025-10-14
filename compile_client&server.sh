rm -f server client

g++ -std=c++17 -O2 -Iinclude checksum.cpp Packet.cpp handshake.cpp server.cpp -o server
g++ -std=c++17 -O2 -Iinclude checksum.cpp Packet.cpp handshake.cpp client.cpp -o client

set -euo pipefail
: "${PORT:=8080}"
SERVER_OUT="${1:-output.txt}"
CLIENT_IN="${2:-input.txt}"

./server "$SERVER_OUT" "$PORT" &
srv=$!

for _ in {1..50}; do
  nc -zu 127.0.0.1 "$PORT" >/dev/null 2>&1 && break
  sleep 0.05
done

./client 127.0.0.1 "$PORT" "$CLIENT_IN" || true

kill -0 "$srv" 2>/dev/null && kill "$srv" 2>/dev/null || true
wait "$srv" 2>/dev/null || true
echo "Done. Output at: $SERVER_OUT"
