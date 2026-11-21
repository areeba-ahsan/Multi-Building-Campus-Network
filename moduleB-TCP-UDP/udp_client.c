#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define PORT 9090
#define BUFFER_SIZE 1024
#define TIMEOUT_SECONDS 2

int main(int argc, char *argv[]) {
    /* Usage: ./udp_client [server_ip] [count] [mode]
       mode: normal (default), skip (skip some sequence numbers), dup (send some twice) */
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int count = (argc > 2) ? atoi(argv[2]) : 5;
    const char *mode = (argc > 3) ? argv[3] : "normal";
    if (count < 1) {
        count = 5;
    }

    int sock;
    struct sockaddr_in server_addr, from_addr;
    socklen_t from_len;
    char message[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    /* 1. Create a UDP socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(1);
    }

    /* 2. Set a receive timeout so recvfrom() does not wait forever */
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
        close(sock);
        exit(1);
    }

    /* 3. Set the server address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid server IP address: %s\n", server_ip);
        close(sock);
        exit(1);
    }

    printf("UDP client: sending %d datagrams to %s:%d (mode: %s, timeout: %d s)\n",
           count, server_ip, PORT, mode, TIMEOUT_SECONDS);

    int seq = 0;
    int replies_ok = 0;
    int timeouts = 0;
    int local_port_shown = 0;

    for (int i = 1; i <= count; i++) {
        seq++;
        /* skip mode: jump over one sequence number every 4th datagram */
        if (strcmp(mode, "skip") == 0 && i % 4 == 0) {
            seq++;
        }

        /* dup mode: send every 3rd datagram twice */
        int sends = (strcmp(mode, "dup") == 0 && i % 3 == 0) ? 2 : 1;

        for (int s = 0; s < sends; s++) {
            /* Message format: SEQ|text */
            snprintf(message, sizeof(message), "%d|Hello from client, datagram %d", seq, i);

            if (sendto(sock, message, strlen(message), 0,
                       (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("sendto failed");
                continue;
            }
            printf("\nSent     -> %s:%d : %s\n", server_ip, PORT, message);

            /* Show our own port once (it is chosen by the operating system) */
            if (!local_port_shown) {
                struct sockaddr_in local_addr;
                socklen_t local_len = sizeof(local_addr);
                if (getsockname(sock, (struct sockaddr *)&local_addr, &local_len) == 0) {
                    printf("(client local port: %d)\n", ntohs(local_addr.sin_port));
                }
                local_port_shown = 1;
            }

            /* Wait for the reply (at most TIMEOUT_SECONDS) */
            from_len = sizeof(from_addr);
            int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&from_addr, &from_len);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("TIMEOUT: no reply for sequence %d within %d seconds "
                           "(the datagram or its reply may be lost)\n",
                           seq, TIMEOUT_SECONDS);
                    timeouts++;
                } else {
                    perror("recvfrom failed");
                }
                continue;
            }
            buffer[n] = '\0';

            char from_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, sizeof(from_ip));
            printf("Received <- %s:%d : %s", from_ip, ntohs(from_addr.sin_port), buffer);

            /* Check that the reply belongs to the request we just sent */
            int reply_seq = -1;
            if (sscanf(buffer, "ACK|%d|", &reply_seq) == 1 && reply_seq == seq) {
                replies_ok++;
            } else {
                printf("WARNING: unexpected reply (wanted ACK for sequence %d)\n", seq);
            }
        }

        usleep(300000);   /* wait 0.3 seconds between datagrams */
    }

    printf("\nSummary: %d replies OK, %d timeouts\n", replies_ok, timeouts);
    close(sock);
    return 0;
}
