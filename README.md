# Multi-Building Campus Network

Design, routing, network services and client-server communication for a three-building college campus.

## Overview

Meridian College has three buildings: Administration, Academic and Services/Data Centre. Each building has a router (R1, R2, R3). The routers form a triangle so that an alternate path exists if one link fails, and R3 is the edge router to a simulated ISP.

The project has two modules:

| Module | What it covers | Tools |
|---|---|---|
| **A: Campus network** | Topology, VLSM addressing, VLANs and trunks, inter-VLAN routing, static routing, RIPv2, OSPF, DHCP relay, NAT and PAT | Cisco Packet Tracer |
| **B: Client-server communication** | A concurrent TCP server and client, a UDP server and client with sequence numbers and timeout, and packet analysis | C (POSIX sockets), gcc, Ubuntu 24.04, Wireshark |

The base network is `172.20.10.0/24`, split with VLSM into seven subnets.

## Repository structure

```
Multi-Building-Campus-Network/
├── module-a-packet-tracer/   Packet Tracer files for the campus network
├── module-b-tcp-udp/         C source code, README and test logs
├── wireshark/                Packet captures (.pcapng)
└── report/                   Final project report
```

## Module A: Campus network (Packet Tracer)

- Three buildings connected by single-mode fibre between the routers and copper inside each building.
- VLSM addressing, largest subnet first.
- 802.1Q trunks and router-on-a-stick inter-VLAN routing on R2.
- Static routing, then RIPv2, then OSPF.
- Central DHCP server in VLAN 40 with DHCP relay on the other routers.
- Static NAT and PAT on R3, the only exit to the ISP.

Open the `.pkt` files in `module-a-packet-tracer/` with Cisco Packet Tracer.

## Module B: Client-server applications

Everything is in [`module-b-tcp-udp/`](module-b-tcp-udp/). It has its own README with the full details.

### Build

```bash
cd module-b-tcp-udp
gcc tcp_server.c -o tcp_server -pthread
gcc tcp_client.c -o tcp_client
gcc udp_server.c -o udp_server
gcc udp_client.c -o udp_client
```

### TCP (port 8080)

The server creates one thread per client, so several clients can be served at the same time. It supports four commands (case-insensitive): `ECHO`, `TIME`, `COUNT` and `UPPER`. Invalid requests get an `ERROR` reply and the server keeps running.

```bash
./tcp_server              # terminal 1
./tcp_client              # terminals 2, 3, 4 ...
```

Example:

```
> UPPER make me big
Server: UPPER: MAKE ME BIG
> COUNT I love networks
Server: COUNT: 15 characters, 3 words
```

### UDP (port 9090)

Each datagram is `SEQ|text` and the server replies `ACK|SEQ|text`. The server reports `[OK]`, `[MISSING]`, `[REPEATED]` and `[OUT OF ORDER]` sequences. The client waits 2 seconds for each reply and prints a `TIMEOUT` message if none arrives.

```bash
./udp_server [N]                        # N = ignore every N-th datagram (simulated loss)
./udp_client [server_ip] [count] [mode] # mode: normal, skip, dup
```

## Wireshark captures

Captured on the loopback interface. Useful display filters: `tcp.port == 8080` and `udp.port == 9090`.

| File | Content |
|---|---|
| `tcp_capture.pcapng` | TCP handshake, commands and connection termination |
| `udp_capture.pcapng` | UDP datagrams and their ACK replies |
| `udp_timeout.pcapng` | UDP with the server ignoring every third datagram |
| `tcp_retransmission.pcapng` | TCP with 30% packet loss: SYN and SYN,ACK retransmissions |



## Requirements

- Cisco Packet Tracer (for Module A)
- Ubuntu 22.04 or 24.04 with `gcc` (for Module B)
- Wireshark (to open the captures)
