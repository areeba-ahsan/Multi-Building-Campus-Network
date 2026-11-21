# Multi-Building Campus Network
### CL3001 — Computer Networks Lab | Semester Project | Fall 2026

**Team:** Sania · Areeba · Ezza  
**Base network:** `172.20.10.0/24` (Group 10)  
**Tools:** Cisco Packet Tracer 8.2.2 · Ubuntu 24.04 LTS (WSL2) · gcc 13.3.0 · Wireshark 4.6.10

---

## Project Overview

Meridian College has three buildings — Administration, Academic and Services/Data Centre — each with its own campus router (R1, R2, R3). The routers form a triangle so that an alternate path is always available if one link fails. R3 is the edge router to a simulated ISP.

The project has two independent modules:

| Module | Scope | Platform |
|---|---|---|
| **A — Campus Network** | Physical topology · VLSM addressing · VLANs & trunks · inter-VLAN routing · static routing · RIPv2 · OSPF (cost + failover) · DHCP relay · static NAT · PAT | Cisco Packet Tracer |
| **B — Client-Server Communication** | Concurrent TCP server/client · UDP server/client with sequence numbers & timeout · Wireshark packet capture & analysis | Ubuntu 24.04 · C (POSIX sockets) · Wireshark |

---

## Repository Structure

```
Multi-Building-Campus-Network/
│
├── moduleB-TCP-UDP/              # Module B — C source code, logs, captures, screenshots
│   ├── tcp_server.c              # Concurrent TCP server (pthread, one thread per client)
│   ├── tcp_client.c              # Interactive TCP client
│   ├── udp_server.c              # UDP server with sequence checking
│   ├── udp_client.c              # UDP client with sequence numbers + 2 s timeout
│   ├── logs/                     # Console output logs for all test runs
│   ├── captures/                 # Wireshark .pcapng capture files (Ezza)
│   ├── screenshots/              # Annotated Wireshark screenshots for the report (Ezza)
│   └── README.md                 # Full compile/run/test/Wireshark instructions
│
└── README.md                     # This file
```

> **Module A** (Packet Tracer `.pkt` files) will be added here by the team before submission.

---

## Module A: Campus Network (Cisco Packet Tracer)

- Three buildings connected by fibre between routers; copper within each building.
- VLSM plan from `172.20.10.0/24` — largest subnet first, no overlapping allocations.
- VLANs 10 (Administration), 20 (Students), 30 (Faculty), 40 (Services).
- 802.1Q trunks and router-on-a-stick for inter-VLAN routing.
- Three routing stages saved as separate files: `Static`, `RIPv2` and `OSPF_Final`.
- OSPF cost challenge: R1–R3 cost 30, R1–R2 cost 10, R2–R3 cost 10 → path change demonstrated.
- Central DHCP server in VLAN 40 with `ip helper-address` relay on remote gateways.
- Static NAT maps the application server to `203.0.113.2`; PAT lets VLANs 10/20/30 share `203.0.113.1`.

---

## Module B: Client-Server Applications

Full details, compile commands and test scenarios are in [`moduleB-TCP-UDP/README.md`](moduleB-TCP-UDP/README.md).

### Quick Build

```bash
cd moduleB-TCP-UDP
gcc tcp_server.c -o tcp_server -pthread
gcc tcp_client.c -o tcp_client
gcc udp_server.c -o udp_server
gcc udp_client.c -o udp_client
```

### TCP — port 8080

One thread per client (pthread), so multiple clients run simultaneously. Supports four commands:

| Command | Example | Reply |
|---|---|---|
| `ECHO` | `ECHO hello` | `ECHO: hello` |
| `TIME` | `TIME` | `TIME: 2026-11-20 12:05:30` |
| `COUNT` | `COUNT hello world` | `COUNT: 11 characters, 2 words` |
| `UPPER` | `UPPER hello` | `UPPER: HELLO` |

Invalid or empty input returns an `ERROR` reply; the server never crashes. Type `quit` to disconnect.

```bash
./tcp_server              # terminal 1
./tcp_client              # terminals 2, 3, 4 (run three to test concurrency)
```

### UDP — port 9090

Datagram format: `SEQ|text` → reply: `ACK|SEQ|text`. Server detects missing, repeated and out-of-order sequences. Client times out after 2 seconds with a `TIMEOUT` message.

```bash
./udp_server [N]                        # ignore every N-th datagram (simulate loss)
./udp_client [server_ip] [count] [mode] # mode: normal | skip | dup
```

---

## Wireshark Analysis (Ezza)

Captures taken on the loopback interface (`lo`) with tshark on Ubuntu; analysed and annotated in Wireshark on Windows.

### Capture files (`moduleB-TCP-UDP/captures/`)

| File | Display filter | What it shows |
|---|---|---|
| `tcp_capture.pcapng` | `tcp.port == 8080` | 3-way handshake · ECHO/TIME/COUNT/UPPER exchange · FIN termination (22 packets) |
| `udp_capture.pcapng` | `udp.port == 9090` | 5 datagrams (SEQ 1–5) and their ACK replies (10 packets) |
| `udp_timeout.pcapng` | `udp.port == 9090` | Server ignoring every 3rd datagram → client TIMEOUT messages |
| `tcp_retransmission.pcapng` | `tcp.analysis.retransmission` | SYN and SYN,ACK retransmitted under 30% simulated packet loss |

### Annotated screenshots (`moduleB-TCP-UDP/screenshots/`)

| File | Annotated content |
|---|---|
| `screenshot_1a_syn_annotated.png` | First SYN: source/dest ports, relative & raw sequence number, SYN flag |
| `screenshot_1b_tcp_annotated.png` | Full TCP session: handshake → data → FIN teardown |
| `screenshot_2_udp_normal_annotated.png` | UDP normal: 5 request/reply pairs, ports and datagram sizes |
| `screenshot_3_udp_timeout_annotated.png` | UDP timeout: missing reply + 2 s TIMEOUT message |
| `screenshot_4_tcp_retransmission_annotated.png` | TCP retransmission: retransmitted SYN/SYN,ACK under packet loss |

### TCP vs UDP — Key Differences

| Feature | TCP | UDP |
|---|---|---|
| Connection setup | 3-way handshake (SYN → SYN,ACK → ACK) | None — connectionless |
| Delivery guarantee | Yes — retransmits lost segments automatically | No — best-effort only |
| Ordering | Guaranteed | May arrive out of order |
| Timeout/recovery | Handled by OS TCP stack | Must be coded in the application |
| Overhead | Higher (headers, ACKs, state) | Lower (minimal 8-byte header) |

---

## Requirements

| Tool | Version | Purpose |
|---|---|---|
| Cisco Packet Tracer | 8.2.2 or later | Module A simulation |
| Ubuntu | 22.04 or 24.04 LTS | Module B — compile and run |
| gcc | any recent | Compile C source files |
| Wireshark / tshark | any recent | Open `.pcapng` captures |

---

## Team Contributions

| Member | Contribution |
|---|---|
| Areeba | Module B — TCP & UDP source code, test logs, initial repo setup |
| Ezza | Module B — Wireshark captures, annotated screenshots, Wireshark analysis & README |
| Sania | Module A — Packet Tracer topology, routing, DHCP/NAT/PAT configuration |
