#!/usr/bin/env python3
# generate_test_topology.py
# Generates routers.txt, hosts.txt, input_packet.txt for a 100-router test.

N = 10 # routers and hosts

def write_routers(fname="routers.txt"):
    with open(fname, "w") as f:
        f.write("# routers (declared)\n")
        for i in range(N):
            f.write(f"Router R{i}\n")
        f.write("\n# backbone chain edges (R0-R1-R2-...-R99)\n")
        for i in range(N-1):
            f.write(f"R{i} R{i+1} 1\n")
        f.write("\n# extra cross links to create a dense-ish topology\n")
        edges = set()
        # add R(i) -- R((i+5)%N) for i where j>i to avoid duplicates
        for i in range(N):
            j = (i + 5) % N
            if i < j:
                w = (i % 7) + 2
                edges.add((i,j,w))
        # add R(i) -- R((3*i+11)%N) if k>i
        for i in range(N):
            k = (3*i + 11) % N
            if i < k:
                w = (i % 5) + 2
                edges.add((i,k,w))
        # add a few longer-range links deterministically
        for i in range(0, N, 10):
            j = (i + 20) % N
            if i < j:
                edges.add((i,j,5))
        # write edges
        for (a,b,w) in sorted(list(edges)):
            f.write(f"R{a} R{b} {w}\n")

def write_hosts(fname="hosts.txt"):
    with open(fname, "w") as f:
        f.write("# hosts: <filename> <ip> <attachedRouter>\n")
        for i in range(N):
            fname_h = f"host{i}.txt"
            ip = f"10.0.0.{i+1}"
            router = f"R{i}"            # attach host_i to R_i
            f.write(f"{fname_h} {ip} {router}\n")

def write_input_packet(fname="input_packet.txt"):
    # Choose a source and destination that are far apart to test routing.
    src = "host0.txt"
    dst_ip = f"10.0.0.{N}"  # host99 (10.0.0.100)
    start_router = "R0"
    with open(fname, "w") as f:
        f.write(f"{src} {dst_ip} {start_router}\n")

if __name__ == "__main__":
    write_routers()
    write_hosts()
    write_input_packet()
    print("Generated routers.txt, hosts.txt, input_packet.txt for", N, "routers/hosts.")
