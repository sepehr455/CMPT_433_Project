#include "../include/client.h"
#include "../../hal/include/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_IP "192.168.6.1"
#define SERVER_PORT 8080

int main(void) {
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
        usleep(100000);
    }

    // Cleanup resources
    cleanup_client();
    cleanup_joystick();

    return 0;
}
