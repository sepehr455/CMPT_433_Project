#include "shutdown.h"
#include <stdatomic.h>

atomic_bool g_shutdown_requested = false;

void request_shutdown(void) {
    g_shutdown_requested = true;
}

bool is_shutdown_requested(void) {
    return g_shutdown_requested;
}
