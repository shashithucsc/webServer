#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>
#include <time.h>
#include "http_helper.h"
#include "file_helper.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 8192
#define CACHE_SIZE 6

typedef struct cacheEntry {
    char file_name[256];
    char *content;
    size_t size;
    time_t lastAccessed;
    struct cacheEntry *next;
} cacheEntry;

cacheEntry *cacheHead = NULL;

void httpResponseBuildFunction(const char *file_name, const char *file_ext, SOCKET client_socket);
void *handleClientFunction(void *arg);
void InitializeCache();
void addToCacheList(const char *file_name, const char *content, size_t size);
cacheEntry *getFromCache(const char *file_name);
void FreeCache();
void sendErrorResponse(SOCKET client_socket, int status);

// Initialize the cache
void InitializeCache() {
    cacheHead = NULL;
}

// Add a file to the cache 
void addToCacheList(const char *file_name, const char *content, size_t size) {
    cacheEntry *NewEntry = (cacheEntry *)malloc(sizeof(cacheEntry));
    strcpy(NewEntry->file_name, file_name);
    NewEntry->content = (char *)malloc(size);
    memcpy(NewEntry->content, content, size);
    NewEntry->size = size;
    NewEntry->lastAccessed = time(NULL);
    NewEntry->next = cacheHead;
    cacheHead = NewEntry;
}

// Get file from the cache
cacheEntry *getFromCache(const char *file_name) {
    cacheEntry *current = cacheHead;
    while (current) {
        if (strcmp(current->file_name, file_name) == 0) {
            current->lastAccessed = time(NULL);
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Free cache memory
void FreeCache() {
    cacheEntry *current = cacheHead;
    while (current) {
        cacheEntry *temp = current;
        current = current->next;
        free(temp->content);
        free(temp);
    }
}

// Send error responses
void sendErrorResponse(SOCKET client_socket, int status) {
    char response[MAX_BUFFER];
    const char *status_text;
    const char *body;

    switch (status) {
        case 400:
            status_text = "400 Bad Request";
            body = "<html><body><h1>400 Bad Request</h1></body></html>";
            break;
        case 404:
            status_text = "404 Not Found";
            body = "<html><body><h1>404 Not Found</h1></body></html>";
            break;
        case 500:
        default:
            status_text = "500 Internal Server Error";
            body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
            break;
    }
    
    snprintf(response, sizeof(response), 
        "HTTP/1.1 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n%s",
        status_text, strlen(body), body);
    send(client_socket, response, strlen(response), 0);
}

// Build and send an HTTP response
void httpResponseBuildFunction(const char *file_name, const char *file_ext, SOCKET client_socket) {
    cacheEntry *cached_entry = getFromCache(file_name);
    
    if (cached_entry) {
        printf("[CACHE HIT] Serving %s\n", file_name);
        char response_header[512];
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
                 get_mime_type(file_ext), cached_entry->size);
        send(client_socket, response_header, strlen(response_header), 0);
        send(client_socket, cached_entry->content, cached_entry->size, 0);
        return;
    }

    FILE *file = fopen(file_name, "rb");
    if (!file) {
        printf("[ERROR] File Not Found: %s\n", file_name);
        sendErrorResponse(client_socket, 404);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = (char *)malloc(file_size);
    fread(file_content, 1, file_size, file);
    fclose(file);

    printf("[SERVING] %s\n", file_name);

    char response_header[512];
    snprintf(response_header, sizeof(response_header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
             get_mime_type(file_ext), file_size);
    
    send(client_socket, response_header, strlen(response_header), 0);
    send(client_socket, file_content, file_size, 0);
    
    addToCacheList(file_name, file_content, file_size);
    free(file_content);
}

void *handleClientFunction(void *arg) {
    SOCKET client_socket = *(SOCKET *)arg;
    free(arg);

    char buffer[MAX_BUFFER];
    int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        sendErrorResponse(client_socket, 400);
        closesocket(client_socket);
        return NULL;
    }
    buffer[received] = '\0';
    
    char file_name[256] = "src/index.html";
    char method[16], requested_path[256];
    
    if (sscanf(buffer, "%s %s", method, requested_path) < 2) {
        sendErrorResponse(client_socket, 400);
        closesocket(client_socket);
        return NULL;
    }

    printf("Request Method: %s, Path: %s\n", method, requested_path);

    if (strcmp(method, "GET") != 0) {
        sendErrorResponse(client_socket, 400);
        closesocket(client_socket);
        return NULL;
    }

    if (strcmp(requested_path, "/") != 0) {
        snprintf(file_name, sizeof(file_name), "src%s", requested_path);
    }

    httpResponseBuildFunction(file_name, get_file_extension(file_name), client_socket);
    closesocket(client_socket);
    return NULL;
}

int main() {
    WSADATA wsa_data;
    SOCKET server_fd, client_fd;
    struct sockaddr_in serverAddress, clientAddress;
    int addressLength = sizeof(clientAddress);
    pthread_t thread_id;

    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(2001);
    bind(server_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    listen(server_fd, 5);
    
    printf("Server running on port 2001...\n");
    InitializeCache();
    
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&clientAddress, &addressLength);
        SOCKET *client_socket = malloc(sizeof(SOCKET));
        *client_socket = client_fd;
        pthread_create(&thread_id, NULL, handleClientFunction, (void *)client_socket);
        pthread_detach(thread_id);
    }
    
    FreeCache();
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
