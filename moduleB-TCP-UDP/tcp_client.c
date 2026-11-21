#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    /* Server IP: take it from the command line, otherwise use 127.0.0.1 (this computer) */
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    /* 1. Create a TCP socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(1);
    }

    /* 2. Set the server address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid server IP address: %s\n", server_ip);
        close(sock);
        exit(1);
    }

    /* 3. Connect to the server (the TCP 3-way handshake happens here) */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed (is the server running?)");
        close(sock);
        exit(1);
    }
    printf("Connected to server %s:%d\n", server_ip, PORT);
    printf("Type a message and press Enter (type 'quit' to exit)\n");

    /* 4. Chat loop */
    while (1) {
        printf("> ");
        fflush(stdout);

        /* Read one line from the keyboard */
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break;   /* Ctrl+D was pressed */
        }

        /* Stop if the user typed 'quit' */
        if (strncmp(input, "quit", 4) == 0) {
            break;
        }

        /* Send the message to the server */
        if (send(sock, input, strlen(input), 0) < 0) {
            perror("send failed");
            break;
        }

        /* Wait for the server's reply */
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (n < 0) {
            perror("recv failed");
            break;
        }
        if (n == 0) {
            printf("Server closed the connection.\n");
            break;
        }
        printf("Server: %s", buffer);
    }

    /* 5. Close the connection */
    close(sock);
    printf("Disconnected.\n");
    return 0;
}
