#include "../include/thread_manager.h"
#include "draw_stuff.h"
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

    DrawStuff_init();
    DisplayScreen1("hey", 2, 2);

    // Keep main thread alive while threads work
    while (1) {
        sleep(1);
    }

    // This will never be reached in current design
    DrawStuff_cleanup();
    cleanup_thread_manager();
    return 0;
}
