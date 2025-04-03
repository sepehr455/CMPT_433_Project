#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static int sock_fd = -1;

// **Initialize the Joystick Client and Connect to Server**
bool init_client(const char* server_ip, int port) {
    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation failed");
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sock_fd);
        return false;
    }

    // Connect to server
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        return false;
    }

    printf("Connected to game server at %s:%d\n", server_ip, port);
    return true;
}

int get_client_socket_fd(void) {
    return sock_fd;
}

void close_client_socket_fd (void) {
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
}


void cleanup_client(void) {
    printf("Cleaning up client socket\n");
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
}
