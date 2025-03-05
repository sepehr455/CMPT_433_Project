#include "../include/client.h"
#include "../../hal/include/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_IP "192.168.1.100"  // Change this to the actual server IP
#define SERVER_PORT 8080           // Same port as the game server

int main() {
    // Initialize the joystick
    init_joystick();

    // Initialize and connect the joystick client to the server
    if (!init_client(SERVER_IP, SERVER_PORT)) {
        fprintf(stderr, "Failed to initialize joystick client\n");
        cleanup_joystick();
        return EXIT_FAILURE;
    }

    // Continuously send joystick data
    while (1) {
        send_joystick_data();
        usleep(100000); // Sleep for 100ms
    }

    // Cleanup resources
    cleanup_client();
    cleanup_joystick();

    return 0;
}
