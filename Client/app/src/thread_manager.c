#include "../include/thread_manager.h"
#include "../include/joystick.h"
#include "../include/rotary_encoder.h"
#include "../include/client.h"
#include "gpio.h"
#include "draw_stuff.h"
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

static pthread_t s_joystick_thread;
static pthread_t s_rotary_thread;
static pthread_t s_transmit_thread;

// threads for receiving server messages and updating LCD
static pthread_t s_receive_thread;
static pthread_t s_lcd_thread;

// Flags
static atomic_bool s_running = false;
static atomic_int s_client_connected = false;

// Store server connection info
static char s_server_ip[16];
static int s_server_port;

static JoystickDirection s_current_direction = NO_DIRECTION;
static volatile int s_rotation_delta = 0;
static volatile bool s_button_pressed = false;
static pthread_mutex_t s_data_mutex = PTHREAD_MUTEX_INITIALIZER;

// store tank health from the server.
static atomic_int s_tank_health = 3;

// Forward declarations
static void send_initial_state(int sock_fd);

// -------------------- INITIAL STATE SENDER --------------------

static void send_initial_state(int sock_fd) {
    char buffer[32] = "NONE";

    pthread_mutex_lock(&s_data_mutex);
    if (s_rotation_delta != 0) {
        char rot_buf[16];
        snprintf(rot_buf, sizeof(rot_buf), ",ROT:%d", s_rotation_delta);
        strcat(buffer, rot_buf);
    }
    if (s_button_pressed) {
        strcat(buffer, ",BTN:1");
    }
    pthread_mutex_unlock(&s_data_mutex);

    send(sock_fd, buffer, strlen(buffer), MSG_NOSIGNAL);
}

static void *joystick_thread_func(void *arg) {
    (void) arg;
    while (s_running) {
        JoystickOutput joystick = read_joystick();

        pthread_mutex_lock(&s_data_mutex);
        s_current_direction = joystick.direction;
        pthread_mutex_unlock(&s_data_mutex);

        usleep(20000); // 50Hz update rate
    }
    return NULL;
}

static void *rotary_thread_func(void *arg) {
    (void) arg;
    while (s_running) {
        // Process all pending events
        RotaryEncoder_processEvents();

        // Read current state
        int rotation = RotaryEncoder_readRotation();
        bool button = RotaryEncoder_readButton();

        if (rotation != 0 || button) {
            pthread_mutex_lock(&s_data_mutex);
            s_rotation_delta += rotation;
            if (button) s_button_pressed = true;
            pthread_mutex_unlock(&s_data_mutex);
        }

        usleep(1000); // 1ms sleep for very responsive handling
    }
    return NULL;
}

static void *transmit_thread_func(void *arg) {
    (void) arg;

    while (s_running) {
        if (s_client_connected) {
            JoystickDirection current_dir;
            int rotation_delta;
            bool button_pressed;
            char buffer[32];
            int retry_count = 0;
            bool send_success = false;

            // Get current state
            pthread_mutex_lock(&s_data_mutex);
            current_dir = s_current_direction;
            rotation_delta = s_rotation_delta;
            button_pressed = s_button_pressed;
            // Reset after reading
            s_rotation_delta = 0;
            s_button_pressed = false;
            pthread_mutex_unlock(&s_data_mutex);

            // Format data
            switch (current_dir) {
                case UP:
                    strcpy(buffer, "UP");
                    break;
                case DOWN:
                    strcpy(buffer, "DOWN");
                    break;
                case LEFT:
                    strcpy(buffer, "LEFT");
                    break;
                case RIGHT:
                    strcpy(buffer, "RIGHT");
                    break;
                default:
                    strcpy(buffer, "NONE");
                    break;
            }

            // Add rotation data if any
            if (rotation_delta != 0) {
                char rot_buf[16];
                snprintf(rot_buf, sizeof(rot_buf), ",ROT:%d", rotation_delta);
                strcat(buffer, rot_buf);
            }

            // Add button data if pressed
            if (button_pressed) {
                strcat(buffer, ",BTN:1");
            }

            // Try sending with retries
            while (retry_count < 3 && !send_success && s_client_connected) {
                if (send(get_client_socket_fd(), buffer, strlen(buffer), MSG_NOSIGNAL) > 0) {
                    send_success = true;
                } else {
                    perror("Failed to send input data");
                    retry_count++;
                    usleep(50000); // Wait 50ms before retry
                }
            }

            if (!send_success) {
                s_client_connected = false;
            }
        } else {
            // Attempt to reconnect using stored IP/port
            if (init_client(s_server_ip, s_server_port)) {
                s_client_connected = true;
                send_initial_state(get_client_socket_fd());
            } else {
                usleep(100000); // Wait 100ms before next reconnect attempt
            }
        }

        usleep(50000); // 20Hz send rate
    }

    return NULL;
}


// Reads from server; if we get "HP:xx", we update s_tank_health.

static void *receive_thread_func(void *arg) {
    (void) arg;

    char recv_buf[64];
    while (s_running) {
        if (!s_client_connected) {
            usleep(100000); // no connection, just wait
            continue;
        }

        int fd = get_client_socket_fd();
        if (fd < 0) {
            usleep(100000);
            continue;
        }

        memset(recv_buf, 0, sizeof(recv_buf));
        int ret = recv(fd, recv_buf, sizeof(recv_buf) - 1, 0);
        if (ret <= 0) {
            if (ret < 0) {
                perror("Error receiving from server");
            } else {
                fprintf(stderr, "Server disconnected.\n");
            }
            s_client_connected = false;
            close_client_socket_fd();
        } else {
            // parse server message
            // e.g. "HP:2", "HP:0"
            if (strncmp(recv_buf, "HP:", 3) == 0) {
                int newHealth = atoi(recv_buf + 3);
                s_tank_health = newHealth;
            }
        }

        usleep(50000);
    }

    return NULL;
}


// Periodically display the tank's current health on the LCD.
static void *lcd_thread_func(void *arg) {
    (void) arg;

    while (s_running) {
        int localHealth = s_tank_health; // atomic read
        DisplayTankStatus(localHealth);

        // Sleep half a second so we aren't spamming the LCD too often
        usleep(500000);
    }

    return NULL;
}

// -------------------- INIT / CLEANUP --------------------

bool init_thread_manager(const char *server_ip, int port) {
    // Store connection info
    strncpy(s_server_ip, server_ip, sizeof(s_server_ip) - 1);
    s_server_ip[sizeof(s_server_ip) - 1] = '\0';
    s_server_port = port;

    // Initialize HAL modules
    DrawStuff_init();
    Gpio_initialize();
    init_joystick();
    RotaryEncoder_init();

    // Initialize client connection
    s_client_connected = init_client(s_server_ip, s_server_port);
    if (s_client_connected) {
        send_initial_state(get_client_socket_fd());
    }

    // Start threads
    s_running = true;

    // Start joystick thread
    if (pthread_create(&s_joystick_thread, NULL, joystick_thread_func, NULL) != 0) {
        perror("Failed to create joystick thread");
        goto error_cleanup;
    }

    // Start rotary encoder thread
    if (pthread_create(&s_rotary_thread, NULL, rotary_thread_func, NULL) != 0) {
        perror("Failed to create rotary thread");
        goto error_cleanup_joystick;
    }

    // Start transmit thread
    if (pthread_create(&s_transmit_thread, NULL, transmit_thread_func, NULL) != 0) {
        perror("Failed to create transmit thread");
        goto error_cleanup_rotary;
    }

    // Receive
    if (pthread_create(&s_receive_thread, NULL, receive_thread_func, NULL) != 0) {
        perror("Failed to create receive thread");
        goto error_cleanup_transmit;
    }

    // LCD
    if (pthread_create(&s_lcd_thread, NULL, lcd_thread_func, NULL) != 0) {
        perror("Failed to create LCD thread");
        goto error_cleanup_receive;
    }

    return true;

    // Error handling

    error_cleanup_receive:
    pthread_cancel(s_receive_thread);
    pthread_join(s_receive_thread, NULL);

    error_cleanup_transmit:
    pthread_cancel(s_transmit_thread);
    pthread_join(s_transmit_thread, NULL);

    error_cleanup_rotary:
    pthread_cancel(s_rotary_thread);
    pthread_join(s_rotary_thread, NULL);

    error_cleanup_joystick:
    pthread_cancel(s_joystick_thread);
    pthread_join(s_joystick_thread, NULL);

    error_cleanup:
    s_running = false;
    if (s_client_connected) {
        cleanup_client();
    }
    Gpio_cleanup();
    DrawStuff_cleanup();
    return false;
}

void cleanup_thread_manager(void) {
    s_running = false;

    // Wait for threads to finish
    pthread_join(s_joystick_thread, NULL);
    pthread_join(s_rotary_thread, NULL);
    pthread_join(s_transmit_thread, NULL);

    //
    pthread_join(s_receive_thread, NULL);
    pthread_join(s_lcd_thread, NULL);

    // Cleanup modules
    cleanup_joystick();
    RotaryEncoder_cleanup();
    Gpio_cleanup();
    DrawStuff_cleanup();
    cleanup_client();
}
