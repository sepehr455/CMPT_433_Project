#ifndef JOYSTICK_CLIENT_H
#define JOYSTICK_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

// Initializes the joystick client (connects to the server)
bool init_client(const char* , int port);

// Sends joystick data to the server
void send_joystick_data(void);

int get_client_socket_fd(void);

// Closes the joystick client connection
void cleanup_client(void);

#endif
