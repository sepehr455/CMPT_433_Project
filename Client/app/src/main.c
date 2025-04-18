#include "../include/thread_manager.h"
#include "shutdown.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_IP "192.168.6.1"
#define SERVER_PORT 8080

int main(void) {
    if (!init_thread_manager(SERVER_IP, SERVER_PORT)) {
        fprintf(stderr, "Failed to initialize thread manager\n");
        return EXIT_FAILURE;
    }

    while (!is_shutdown_requested()) {
        sleep(1);
    }

    // This will never be reached in current design
    cleanup_thread_manager();
    return 0;
}
