# README — Link-State Data-Plane Simulator
A small C++17 program that simulates packet forwarding using link-state routing (Dijkstra) and per-router FIBs.

## build
```
g++ -std=c++17 -O2 ls_data_plane_sim.cpp -o ls_sim
```
## run
```
./ls_sim input_packet.txt
```
The program expects these files in the current directory:

routers.txt — defines routers and weighted links

hosts.txt — defines hosts, their IPs, and attached routers

input_packet.txt — defines the packet to send

Defaults are auto-generated if missing.

## File Formats

`routers.txt`
```
Router R0
R0 R1 10
Link R1 R2 5
```

* Either Router <name> or <A> <B> <weight> or Link <A> <B> <weight>

* Links are undirected.

`hosts.txt`
```
hostA.txt 10.0.0.1 R0
hostB.txt 10.0.0.2 R2
```

`input_packet.txt`
```
<src_host_file> <dst_ip> [start_router]
```

Example:

```hostA.txt 10.0.0.2```

## What the Program Does

1. Loads topology from routers.txt.

2. Loads hosts and their attachments from hosts.txt.

3. Reads payload from the source host file.

4. For every router, computes shortest paths (Dijkstra) and builds FIBs.

5. Simulates packet forwarding hop-by-hop using FIBs and TTL=64.

6. On delivery, appends the payload and hop log to the destination host file.

7. Prints hop-by-hop simulation results and all router FIBs.

### Output

* Console: hop log + delivery status + FIB tables

* Destination host file: appended payload + timestamp + hop path

Notes

* Packet drops if unreachable, no FIB entry, or TTL expires.

* Next-hop routing only; no ARP, no queues, no link failures.

* Everything is file-based; host files simulate send/receive buffers.