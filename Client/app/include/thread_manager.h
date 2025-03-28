#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <stdbool.h>

/**
 * Module for managing all input threads and data transmission
 */

bool init_thread_manager(const char* server_ip, int port);
void cleanup_thread_manager(void);

#endif
