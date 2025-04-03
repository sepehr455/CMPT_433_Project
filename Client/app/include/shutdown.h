#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <stdatomic.h>
#include <stdbool.h>

extern atomic_bool g_shutdown_requested;

void request_shutdown(void);
bool is_shutdown_requested(void);

#endif
