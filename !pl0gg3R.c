// Language: C11, File: iplogger.c, Target: Linux
// Compile: gcc -O2 -o iplogger iplogger.c -pthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_BUF 4096

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[MAX_BUF];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Get client IP
    getpeername(client_sock, (struct sockaddr*)&addr, &addr_len);
    char* client_ip = inet_ntoa(addr.sin_addr);

    // Read HTTP request
    int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';

        // Log IP to file
        FILE* f = fopen("ips.txt", "a");
        if (f) {
            fprintf(f, "[+] Connection from: %s\n", client_ip);
            fclose(f);
        }

        // Send an empty 1x1 pixel response (transparent)
        const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/gif\r\n"
        "Content-Length: 43\r\n"
        "Connection: close\r\n\r\n"
        "GIF89a\x01\x00\x01\x00\x80\x00\x00\x00\x00\x00\xff\xff\xff!\xf9\x04\x01\x00\x00\x00\x00,\x00\x00\x00\x00\x01\x00\x01\x00\x00\x02\x02D\x01\x00;";
        send(client_sock, response, strlen(response), 0);
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) return 1;

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) return 1;

    listen(server_sock, 10);
    printf("[*] IP Logger listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock == -1) continue;

        pthread_t t;
        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }
    return 0;
}
