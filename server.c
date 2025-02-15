#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>
#include "http_helper.h"
#include "file_helper.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 8192  

void build_http_response(const char *file_name, const char *file_ext, SOCKET client_socket);
void *handle_client(void *arg);

void build_http_response(const char *file_name, const char *file_ext, SOCKET client_socket) {
    FILE *file = fopen(file_name, "rb");

    if (!file) {
        char response_header[512];
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/html\r\n\r\n"
                 "<html><body><h1>404 Not Found</h1></body></html>");
        send(client_socket, response_header, strlen(response_header), 0);
        return;
    }

    const char *content_type = get_mime_type(file_ext);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char response_header[512];
    snprintf(response_header, sizeof(response_header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n",
             content_type, file_size);
    
    send(client_socket, response_header, strlen(response_header), 0);

    char buffer[8096];  // Increase buffer size
size_t bytes_read;
while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    send(client_socket, buffer, bytes_read, 0);
}


    fclose(file);
}

void *handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET *)arg;
    free(arg);

    char buffer[1024];
    recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    char file_name[256] = "src/index.html";  
    char method[16], requested_path[256];

    sscanf(buffer, "%s %s", method, requested_path);

    if (strcmp(requested_path, "/") != 0) {
        snprintf(file_name, sizeof(file_name), "src%s", requested_path);
    }

    const char *file_ext = get_file_extension(file_name);
    build_http_response(file_name, file_ext, client_socket);

    closesocket(client_socket);
    return NULL;
}

int main() {
    WSADATA wsa_data;
    SOCKET server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    pthread_t thread_id;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        WSACleanup();
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    printf("Server running on port 8080...\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            printf("Failed to accept connection\n");
            continue;
        }

        SOCKET *client_socket = malloc(sizeof(SOCKET));
        *client_socket = client_fd;

        pthread_create(&thread_id, NULL, handle_client, (void *)client_socket);
        pthread_detach(thread_id);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
