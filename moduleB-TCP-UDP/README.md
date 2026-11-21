# CL3001 Computer Networks Lab — Module B: Client-Server Communication
**Team:** Sania, Areeba & Ezza  
**Platform:** Ubuntu 24.04 LTS (WSL2), gcc 13.3.0, tshark/Wireshark 4.2.2 (capture); Wireshark 4.6.10 on Windows (analysis and annotation)

---

## Files

| File | Purpose |
|---|---|
| `tcp_server.c` | Concurrent TCP server (one thread per client, pthread) |
| `tcp_client.c` | TCP client (interactive command prompt) |
| `udp_server.c` | UDP server with sequence checking (OK / MISSING / REPEATED / OUT OF ORDER) |
| `udp_client.c` | UDP client with sequence numbers and 2 s receive timeout |
| `logs/` | Console logs of all TCP and UDP test runs |
| `captures/` | Wireshark packet captures (.pcapng) — Ezza's Wireshark work |
| `screenshots/` | Annotated Wireshark screenshots used in the report — Ezza's Wireshark work |

---

## Compile

```bash
gcc tcp_server.c -o tcp_server -pthread
gcc tcp_client.c -o tcp_client
gcc udp_server.c -o udp_server
gcc udp_client.c -o udp_client
```

---

## TCP Application (port 8080)

Run the server, then start three clients in three separate terminals:

```bash
./tcp_server
./tcp_client [server_ip]      # default 127.0.0.1
```

### Supported commands (case-insensitive)

| Command | Example input | Server reply |
|---|---|---|
| `ECHO` | `ECHO hello` | `ECHO: hello` |
| `TIME` | `TIME` | `TIME: 2026-11-20 12:05:30` |
| `COUNT` | `COUNT hello world` | `COUNT: 11 characters, 2 words` |
| `UPPER` | `UPPER hello` | `UPPER: HELLO` |

- Empty, malformed or unsupported requests get an `ERROR` reply; the server never crashes.
- Type `quit` in the client to disconnect cleanly.
- The server prints the IP address and port of every connecting client.

---

## UDP Application (port 9090)

```bash
./udp_server [N]                        # N = ignore every N-th datagram (simulate loss)
./udp_client [server_ip] [count] [mode] # modes: normal (default) | skip | dup
```

- **Message format:** `SEQ|text`  — **Reply format:** `ACK|SEQ|text`
- The client waits **2 seconds** for each reply and prints a `TIMEOUT` message if none arrives.
- The server reports `[OK]`, `[MISSING]`, `[REPEATED]` and `[OUT OF ORDER]` per datagram.

---

## Test Scenarios

| Test | Command | Expected result |
|---|---|---|
| 3 TCP clients | start 3 × `./tcp_client` | server shows 3 different client ports |
| Bad input | send `DANCE now` or an empty line | `ERROR` reply, server keeps running |
| Abrupt disconnect | Ctrl+C in a client | server prints `disconnected`, keeps running |
| UDP normal | `./udp_client 127.0.0.1 5` | 5 ACKs, no timeouts |
| UDP timeout | `./udp_server 3`, then `./udp_client 127.0.0.1 6` | TIMEOUT messages on seq 3 and 6 |
| UDP missing seq | `./udp_client 127.0.0.1 8 skip` | server prints `[MISSING]` |
| UDP repeated | `./udp_client 127.0.0.1 6 dup` | server prints `[REPEATED]` |

Logs of all these runs are in the `logs/` folder.

---

## Wireshark Analysis (Ezza)

All captures were taken on the loopback interface (`lo`) with tshark on Ubuntu, with both client and server running on `127.0.0.1`. The `.pcapng` files were then opened in Wireshark on Windows for analysis and annotation.

### Capture files (`captures/`)

| File | Traffic | Display filter | What it shows |
|---|---|---|---|
| `tcp_capture.pcapng` | TCP port 8080 | `tcp.port == 8080` | 22 packets: 3-way handshake, ECHO/TIME/COUNT/UPPER/error exchange, FIN termination |
| `udp_capture.pcapng` | UDP port 9090 | `udp.port == 9090` | 10 packets: 5 datagrams with sequence numbers 1–5 and their ACK replies |
| `udp_timeout.pcapng` | UDP port 9090 | `udp.port == 9090` | Run with `./udp_server 3`: sequences 3 and 6 get no reply (timeout scenario) |
| `tcp_retransmission.pcapng` | TCP port 8080 | `tcp.analysis.retransmission` | Run with 30% packet loss on `lo`: SYN and SYN,ACK retransmitted |

### How the captures were made

```bash
# Start capture first, then run server + client in other terminals, then Ctrl+C to stop

sudo tshark -i lo -f "tcp port 8080" -w captures/tcp_capture.pcapng
sudo tshark -i lo -f "udp port 9090" -w captures/udp_capture.pcapng
sudo tshark -i lo -f "udp port 9090" -w captures/udp_timeout.pcapng
sudo tshark -i lo -f "tcp port 8080" -w captures/tcp_retransmission.pcapng
```

For `tcp_retransmission.pcapng`, 30% packet loss was added to the loopback interface before the run:

```bash
sudo tc qdisc add dev lo root netem loss 30%   # add loss
# ... run server and client ...
sudo tc qdisc del dev lo root                   # remove loss
```

To read a capture in the terminal:

```bash
tshark -r captures/tcp_capture.pcapng
tshark -r captures/tcp_retransmission.pcapng -Y "tcp.analysis.retransmission"
```

### Annotated screenshots (`screenshots/`)

| File | Content |
|---|---|
| `screenshot_1a_syn_annotated.png` | First SYN packet: source/dest ports, relative and raw sequence number, SYN flag |
| `screenshot_1b_tcp_annotated.png` | Full TCP session: 3-way handshake, data exchange and FIN/FIN-ACK termination |
| `screenshot_2_udp_normal_annotated.png` | UDP normal run: 5 request/reply pairs, ports and datagram sizes annotated |
| `screenshot_3_udp_timeout_annotated.png` | UDP timeout: missing server replies and 2 s client TIMEOUT message |
| `screenshot_4_tcp_retransmission_annotated.png` | TCP retransmission: retransmitted SYN and SYN,ACK under 30% loss |

### Key Observations

- **TCP handshake:** Client port `39424` → server port `8080`. SYN (Seq=0) → SYN,ACK (Seq=0, Ack=1) → ACK (Seq=1, Ack=1). Sequence numbers shown are relative. Connection closed cleanly with FIN,ACK / FIN,ACK / ACK.
- **UDP exchange:** Client port `41845` → server port `9090`. No handshake or teardown. Requests are ~31 bytes (`SEQ|text`), replies are ~36 bytes (`ACK|SEQ|text`). Sequence numbers are application-level, not from UDP itself.
- **UDP timeout:** Server ignores every 3rd datagram; no reply is sent and the client prints `TIMEOUT` after 2 seconds. UDP does not automatically retransmit.
- **TCP retransmission:** Under 30% packet loss, the lost SYN was retransmitted automatically by the OS TCP stack after ~1 second, with no action required from the application code.

### TCP vs UDP Comparison

| Feature | TCP | UDP |
|---|---|---|
| Connection | 3-way handshake required | Connectionless, no handshake |
| Reliability | Guaranteed delivery + retransmission | Best-effort, no retransmission |
| Ordering | Bytes delivered in order | Datagrams may arrive out of order |
| Overhead | Higher (headers, ACKs, state) | Lower (minimal header) |
| Timeout/recovery | Handled by OS TCP stack | Must be implemented in application |
| Use case | ECHO/TIME/COUNT/UPPER server (this project) | Datagram exchange (this project) |
