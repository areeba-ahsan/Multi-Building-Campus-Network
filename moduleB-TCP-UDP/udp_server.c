#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9090
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    /* Optional argument: ignore every N-th datagram to simulate packet loss */
    int drop_every = (argc > 1) ? atoi(argv[1]) : 0;

    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    char reply[BUFFER_SIZE + 64];
    int last_seq = 0;         /* highest sequence number seen so far */
    int received_count = 0;   /* total datagrams received */

    /* 1. Create a UDP socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Set the server address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind the socket to the port (no listen or accept in UDP) */
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sock);
        exit(1);
    }

    printf("UDP server listening on port %d... (press Ctrl+C to stop)\n", PORT);
    if (drop_every > 0) {
        printf("Simulated loss ON: ignoring every %d-th datagram\n", drop_every);
    }

    /* 4. Main loop: receive a datagram, check its sequence number, reply */
    while (1) {
        client_len = sizeof(client_addr);
        int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }
        buffer[n] = '\0';
        received_count++;

        /* Print the client's IP address and port */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        printf("\nReceived from %s:%d -> %s\n", client_ip, client_port, buffer);

        /* Read the sequence number and the message (format: SEQ|message) */
        int seq = 0;
        char text[BUFFER_SIZE];
        text[0] = '\0';
        if (sscanf(buffer, "%d|%1000[^\n]", &seq, text) < 1) {
            snprintf(reply, sizeof(reply), "ERROR|bad format, expected SEQ|message\n");
            sendto(sock, reply, strlen(reply), 0,
                   (struct sockaddr *)&client_addr, client_len);
            printf("Malformed datagram, error reply sent\n");
            continue;
        }

        /* Simulated packet loss: ignore this datagram completely */
        if (drop_every > 0 && received_count % drop_every == 0) {
            printf("[SIMULATED LOSS] ignoring sequence %d, no reply sent\n", seq);
            continue;
        }

        /* A new client run starts again from sequence 1 */
        if (seq == 1 && last_seq > 1) {
            printf("[NEW SESSION] sequence numbers restarted from 1\n");
            last_seq = 0;
        }

        /* Check the sequence number */
        if (seq == last_seq) {
            printf("[REPEATED] sequence %d was received again\n", seq);
        } else if (seq > last_seq + 1) {
            printf("[MISSING] expected %d but got %d (missing: %d to %d)\n",
                   last_seq + 1, seq, last_seq + 1, seq - 1);
        } else if (seq < last_seq) {
            printf("[OUT OF ORDER] got %d after %d\n", seq, last_seq);
        } else {
            printf("[OK] sequence %d is in order\n", seq);
        }
        if (seq > last_seq) {
            last_seq = seq;
        }

        /* Send the acknowledgement back to the same client */
        snprintf(reply, sizeof(reply), "ACK|%d|%s\n", seq, text);
        if (sendto(sock, reply, strlen(reply), 0,
                   (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("sendto failed");
        } else {
            printf("Reply sent to %s:%d -> ACK for sequence %d\n",
                   client_ip, client_port, seq);
        }
    }

    close(sock);
    return 0;
}
