# CL3001 Computer Networks Lab - Module B: Client-Server Communication
# Sania ,Areeba & Ezza
Platform: Ubuntu 24.04 LTS, gcc 13.3.0, Wireshark 4.2.2

## Files

| File | Purpose |
|---|---|
| tcp_server.c | Concurrent TCP server (one thread per client) |
| tcp_client.c | TCP client |
| udp_server.c | UDP server with sequence checking |
| udp_client.c | UDP client with sequence numbers and receive timeout |
| logs/ | Test logs of the TCP and UDP runs |

## Compile

    gcc tcp_server.c -o tcp_server -pthread
    gcc tcp_client.c -o tcp_client
    gcc udp_server.c -o udp_server
    gcc udp_client.c -o udp_client

## TCP application (port 8080)

Run the server, then start three clients in three terminals:

    ./tcp_server
    ./tcp_client [server_ip]      (default 127.0.0.1)

Supported commands (case-insensitive):

| Command | Example | Reply |
|---|---|---|
| ECHO | ECHO hello | ECHO: hello |
| TIME | TIME | TIME: 2026-11-20 12:05:30 |
| COUNT | COUNT hello world | COUNT: 11 characters, 2 words |
| UPPER | UPPER hello | UPPER: HELLO |

Empty, malformed or unsupported requests get an ERROR reply and the server
keeps running. Type quit in the client to disconnect.
The server prints the IP address and port of every client that connects.

## UDP application (port 9090)

    ./udp_server [N]              (N = ignore every N-th datagram, to simulate loss)
    ./udp_client [server_ip] [count] [mode]

Modes: normal (default), skip (skips some sequence numbers), dup (sends some datagrams twice).
Message format: SEQ|text. Reply format: ACK|SEQ|text.
The client waits 2 seconds for each reply and prints a TIMEOUT message if none arrives.
The server reports [OK], [MISSING], [REPEATED] and [OUT OF ORDER] sequences.

## Test scenarios

| Test | Command | Expected result |
|---|---|---|
| 3 TCP clients | start 3 x ./tcp_client | server shows 3 different client ports |
| Bad input | send DANCE now, or an empty line | ERROR reply, server keeps running |
| Abrupt disconnect | Ctrl+C in a client | server prints disconnected, keeps running |
| UDP normal | ./udp_client 127.0.0.1 5 | 5 ACKs, no timeouts |
| UDP timeout | ./udp_server 3, then ./udp_client 127.0.0.1 6 | TIMEOUT messages |
| UDP missing | ./udp_client 127.0.0.1 8 skip | server prints [MISSING] |
| UDP repeated | ./udp_client 127.0.0.1 6 dup | server prints [REPEATED] |

Logs of all these runs are in the logs/ folder.

## Wireshark

Captures were taken on the loopback interface (lo).
Display filters: tcp.port == 8080 and udp.port == 9090.
Capture files: [file names, for example tcp_capture.pcapng and udp_capture.pcapng]


