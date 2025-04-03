#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool init_thread_manager(const char *server_ip, int port);

void cleanup_thread_manager(void);

#ifdef __cplusplus
}
#endif
