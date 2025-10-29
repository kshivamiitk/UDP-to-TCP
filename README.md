# UDP-to-TCP

This project implements a reliable, TCP-like transport protocol on top of UDP sockets.  It provides a minimal client/server pair that perform a three-way handshake, exchange data reliably with retransmissions and congestion control, and compute checksums to detect bit errors.

## Building and running

```bash
chmod +x compile_client&server.sh

./compile_client&server.sh
```

```bash
./server <output_file> [port]

./client <server_ip> <server_port> <input_file>
```

eg:
```bash
./server output.txt 8080

./client 127.0.0.1 8080 input.txt

```
## checking congestion

```bash
chmod +x test_congestion_control.sh
./test_congestion_control.sh
```

## Source overview

### `checksum.cpp`
Implements a 16-bit Internet-style checksum that sums consecutive 16-bit words, folds carry bits, and returns the one's complement of the result.  Both packet construction and validation use this helper to detect transmission errors.

### `Packet.cpp`
Defines helpers for serialising and parsing protocol packets.  `build_packet` assembles headers in network byte order, appends payload bytes, and fills in the checksum field.  `parse_packet` verifies packet length, recomputes the checksum, extracts header fields, and returns the payload vector used by higher-level logic.【F:Packet.cpp†L1-L47】

### `handshake.cpp`
Contains both client and server sides of a TCP-style three-way handshake.  `perform_handshake` repeatedly sends SYN packets, waits with a timeout for a SYN-ACK, and responds with an ACK upon success.  `handle_handshake` listens for incoming SYNs, replies with SYN-ACK, and confirms completion after receiving the final ACK, tolerating retransmissions when timeouts occur.

### `client.cpp`
Implements the file-sending client.  After completing the handshake it:
- Reads the input file in fixed-size chunks and wraps each chunk in a data packet.
- Tracks in-flight segments and maintains a retransmission queue with backoff timers.
- Processes acknowledgements to slide the send window, trigger fast retransmit on duplicate ACKs, and send a FIN when reaching end-of-file.
- Integrates with the congestion controller to respect both congestion and advertised receive window limits.

### `server.cpp`
Runs the receiving endpoint.  Once the handshake finishes it:
- Writes in-order payloads to the output file and buffers out-of-order segments until gaps close.
- Acknowledges the highest contiguous sequence number received to drive the sender's sliding window.
- Detects the FIN flag to terminate the transfer cleanly and prepare for the next client.

### `include/congestion.h`
Provides the additive-increase/multiplicative-decrease congestion controller.  It keeps track of congestion window, slow-start threshold, duplicate ACK counters, and fast-recovery state.  The client consults `sendAllowance` before sending, calls `onNewAck`, `onDupAck`, and `onTimeout` to adjust the window, and resets duplicate ACK counters when appropriate.