#include "../include/client.h"
#include "../../hal/include/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SEND_INTERVAL_MS 100  // Send data every 100ms

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

// **Send Joystick Data to Server**
void send_joystick_data(void) {
    if (sock_fd == -1) {
        fprintf(stderr, "Joystick client is not connected!\n");
        return;
    }

    JoystickOutput joystick = read_joystick();
    char buffer[16];  // Enough to hold direction strings

    // Convert joystick direction to a string
    switch (joystick.direction) {
        case UP: strcpy(buffer, "UP"); break;
        case DOWN: strcpy(buffer, "DOWN"); break;
        case LEFT: strcpy(buffer, "LEFT"); break;
        case RIGHT: strcpy(buffer, "RIGHT"); break;
        default: strcpy(buffer, "NONE"); break;
    }

    // Send data to server
    if (send(sock_fd, buffer, strlen(buffer), 0) == -1) {
        perror("Failed to send joystick data");
    }
}

// **Cleanup the Joystick Client**
void cleanup_client(void) {
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
}
