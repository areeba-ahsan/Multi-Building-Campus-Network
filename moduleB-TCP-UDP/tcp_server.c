#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define REPLY_SIZE 1024

/* Information we pass to each client thread */
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

/* Number of clients connected right now (shared by all threads) */
static int active_clients = 0;
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Read one request such as "UPPER hello" and write the reply into 'reply'.
 * Supported commands: ECHO, TIME, COUNT, UPPER.
 * Bad requests never crash the server, they get an ERROR reply instead.
 */
static void process_request(const char *request, char *reply, size_t reply_size) {
    char line[BUFFER_SIZE];
    snprintf(line, sizeof(line), "%s", request);

    /* Remove trailing newline, carriage return and spaces */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
        line[--len] = '\0';
    }

    /* Skip leading spaces */
    char *cmd = line;
    while (*cmd == ' ') {
        cmd++;
    }

    /* Empty request */
    if (*cmd == '\0') {
        snprintf(reply, reply_size, "ERROR: empty request\n");
        return;
    }

    /* Split into command and argument at the first space */
    char *arg = cmd;
    while (*arg != '\0' && *arg != ' ') {
        arg++;
    }
    if (*arg != '\0') {
        *arg = '\0';   /* end the command word here */
        arg++;
        while (*arg == ' ') {
            arg++;
        }
    }
    /* Now 'cmd' is the command and 'arg' is the rest (it may be empty) */

    if (strcasecmp(cmd, "ECHO") == 0) {
        if (*arg == '\0') {
            snprintf(reply, reply_size, "ERROR: ECHO needs some text\n");
        } else {
            snprintf(reply, reply_size, "ECHO: %s\n", arg);
        }
    }
    else if (strcasecmp(cmd, "TIME") == 0) {
        time_t now = time(NULL);
        struct tm tm_info;
        char time_text[64];
        localtime_r(&now, &tm_info);
        strftime(time_text, sizeof(time_text), "%Y-%m-%d %H:%M:%S", &tm_info);
        snprintf(reply, reply_size, "TIME: %s\n", time_text);
    }
    else if (strcasecmp(cmd, "COUNT") == 0) {
        if (*arg == '\0') {
            snprintf(reply, reply_size, "ERROR: COUNT needs some text\n");
        } else {
            size_t chars = strlen(arg);
            int words = 0;
            int in_word = 0;
            for (const char *p = arg; *p != '\0'; p++) {
                if (*p == ' ') {
                    in_word = 0;
                } else if (!in_word) {
                    in_word = 1;
                    words++;
                }
            }
            snprintf(reply, reply_size, "COUNT: %zu characters, %d words\n", chars, words);
        }
    }
    else if (strcasecmp(cmd, "UPPER") == 0) {
        if (*arg == '\0') {
            snprintf(reply, reply_size, "ERROR: UPPER needs some text\n");
        } else {
            for (char *p = arg; *p != '\0'; p++) {
                *p = (char)toupper((unsigned char)*p);
            }
            snprintf(reply, reply_size, "UPPER: %s\n", arg);
        }
    }
    else {
        snprintf(reply, reply_size,
                 "ERROR: unsupported command '%s'. Use ECHO, TIME, COUNT or UPPER\n", cmd);
    }
}

/* This function runs in its own thread, one thread per client */
void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    int client_fd = info->client_fd;
    char client_ip[INET_ADDRSTRLEN];
    int client_port = ntohs(info->client_addr.sin_port);
    char buffer[BUFFER_SIZE];
    char reply[REPLY_SIZE];

    inet_ntop(AF_INET, &info->client_addr.sin_addr, client_ip, sizeof(client_ip));
    free(info);   /* we copied what we need, so free the memory */

    /* Increase the client counter (lock so two threads do not change it together) */
    pthread_mutex_lock(&clients_lock);
    active_clients++;
    printf("[+] Client connected: %s:%d (active clients: %d)\n",
           client_ip, client_port, active_clients);
    pthread_mutex_unlock(&clients_lock);

    /* Communication loop for this client */
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (n < 0) {
            perror("recv failed");
            break;
        }
        if (n == 0) {
            break;   /* client closed the connection */
        }

        printf("[%s:%d] Request: %s", client_ip, client_port, buffer);

        /* Work out the reply for this request */
        process_request(buffer, reply, sizeof(reply));

        /* MSG_NOSIGNAL stops the server from crashing if the client has already gone */
        if (send(client_fd, reply, strlen(reply), MSG_NOSIGNAL) < 0) {
            break;
        }
    }

    close(client_fd);

    /* Decrease the client counter */
    pthread_mutex_lock(&clients_lock);
    active_clients--;
    printf("[-] Client disconnected: %s:%d (active clients: %d)\n",
           client_ip, client_port, active_clients);
    pthread_mutex_unlock(&clients_lock);

    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    /* 1. Create a TCP socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Set the server address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }

    /* 4. Listen */
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(1);
    }
    printf("Server listening on port %d... (press Ctrl+C to stop)\n", PORT);

    /* 5. Main loop: accept clients forever, one new thread for each */
    while (1) {
        client_info_t *info = malloc(sizeof(client_info_t));
        if (info == NULL) {
            perror("malloc failed");
            continue;
        }

        socklen_t client_len = sizeof(info->client_addr);
        info->client_fd = accept(server_fd, (struct sockaddr *)&info->client_addr, &client_len);
        if (info->client_fd < 0) {
            perror("accept failed");
            free(info);
            continue;   /* do not stop the server, wait for the next client */
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, info) != 0) {
            perror("pthread_create failed");
            close(info->client_fd);
            free(info);
            continue;
        }
        pthread_detach(tid);   /* the thread cleans itself up when it finishes */
    }

    close(server_fd);
    return 0;
}
