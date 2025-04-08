#include "../include/thread_manager.h"
#include "../include/joystick.h"
#include "../include/rotary_encoder.h"
#include "../include/client.h"
#include "gpio.h"
#include "draw_stuff.h"
#include "sound_effects.h"
#include "shutdown.h"
#include "led.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

#include "../include/accelerometer.h"
#include <time.h>

static pthread_t s_joystick_thread;
static pthread_t s_rotary_thread;
static pthread_t s_transmit_thread;
static pthread_t s_receive_thread;
static pthread_t s_lcd_thread;

static pthread_t s_accelerometer_thread;
static atomic_bool s_newCheatRequest = false;

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

// Initial State Sender
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
    while (s_running && !is_shutdown_requested()) {
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
    while (s_running && !is_shutdown_requested()) {
        RotaryEncoder_processEvents();

        // Read current state
        int rotation = RotaryEncoder_readRotation();
        bool button = RotaryEncoder_readButton();

        if (rotation != 0 || button) {
            pthread_mutex_lock(&s_data_mutex);
            s_rotation_delta += rotation;
            if (button) {
                s_button_pressed = true;

                // Play shooting sound
                SoundEffects_playShoot();
            }
            pthread_mutex_unlock(&s_data_mutex);
        }

        usleep(1000); // 1ms sleep for very responsive handling
    }
    return NULL;
}

// Logic for the accelerometer cheat
typedef enum {
    TILT_FLAT,
    TILT_DOWN,
    TILT_UP,
    TILT_LEFT,
    TILT_RIGHT
} TiltDirection;

static TiltDirection getTiltDirectionFromAccelerometer(void) {
    float x_tilt, y_tilt;
    Accelerometer_getTiltDirection(&x_tilt, &y_tilt);

    const float TILT_THRESHOLD = 0.5f;

    if (x_tilt < -TILT_THRESHOLD) {
        return TILT_DOWN;
    } else if (x_tilt > TILT_THRESHOLD) {
        return TILT_UP;
    } else if (y_tilt > TILT_THRESHOLD) {
        return TILT_RIGHT;
    } else if (y_tilt < -TILT_THRESHOLD) {
        return TILT_LEFT;
    } else {
        return TILT_FLAT;
    }
}

// New cheat sequence: DOWN → FLAT → LEFT → FLAT → RIGHT → FLAT
static const TiltDirection CHEAT_SEQUENCE[] = {
        TILT_DOWN, TILT_FLAT, TILT_LEFT, TILT_FLAT, TILT_RIGHT, TILT_FLAT
};
static const int CHEAT_SEQUENCE_LENGTH = 6;
static const double CHEAT_TIMEOUT_SECONDS = 5.0;

static void *accelerometer_thread_func(void *arg) {
    (void) arg;

    int cheatIndex = 0;
    bool inCheatSequence = false;
    struct timespec startTime;

    while (s_running && !is_shutdown_requested()) {
        TiltDirection currentTilt = getTiltDirectionFromAccelerometer();

        if (!inCheatSequence) {
            if (currentTilt == CHEAT_SEQUENCE[0]) {
                inCheatSequence = true;
                cheatIndex = 1;
                clock_gettime(CLOCK_MONOTONIC, &startTime);
                printf("[ACCEL] Cheat sequence started. Step 1/%d matched.\n",
                       CHEAT_SEQUENCE_LENGTH);
            }
        } else {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed = (now.tv_sec - startTime.tv_sec)
                             + (now.tv_nsec - startTime.tv_nsec) / 1e9;

            if (elapsed > CHEAT_TIMEOUT_SECONDS) {
                inCheatSequence = false;
                cheatIndex = 0;
                printf("[ACCEL] Cheat sequence timed out (>%.1f s). Resetting.\n",
                       CHEAT_TIMEOUT_SECONDS);
            } else {
                if (currentTilt == CHEAT_SEQUENCE[cheatIndex]) {
                    cheatIndex++;
                    printf("[ACCEL] Cheat step %d/%d matched.\n",
                           cheatIndex, CHEAT_SEQUENCE_LENGTH);

                    if (cheatIndex == CHEAT_SEQUENCE_LENGTH) {
                        if (elapsed <= CHEAT_TIMEOUT_SECONDS) {
                            pthread_mutex_lock(&s_data_mutex);
                            s_newCheatRequest = true;
                            pthread_mutex_unlock(&s_data_mutex);
                            printf("[ACCEL] Cheat code recognized! Will send \"CHEAT\".\n");
                        }
                        inCheatSequence = false;
                        cheatIndex = 0;
                    }
                }
            }
        }

        usleep(200000);
    }
    return NULL;
}
// ----------------------------------------------------------------------

static void *transmit_thread_func(void *arg) {
    (void) arg;
    while (s_running && !is_shutdown_requested()) {
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
                    usleep(50000);
                }
            }

            if (!send_success) {
                s_client_connected = false;
            }

            if (s_client_connected) {
                bool localCheat = false;
                pthread_mutex_lock(&s_data_mutex);
                localCheat = s_newCheatRequest;
                if (localCheat) {
                    s_newCheatRequest = false;
                }
                pthread_mutex_unlock(&s_data_mutex);

                if (localCheat) {
                    retry_count = 0;
                    send_success = false;
                    while (retry_count < 3 && !send_success && s_client_connected) {
                        if (send(get_client_socket_fd(), "CHEAT", 5, MSG_NOSIGNAL) > 0) {
                            send_success = true;
                            printf("[ACCEL] Cheat code sent to server.\n");
                        } else {
                            perror("Failed to send cheat code");
                            retry_count++;
                            usleep(50000);
                        }
                    }
                    if (!send_success) {
                        s_client_connected = false;
                    }
                }
            }
        } else {
            // Attempt to reconnect using stored IP/port
            if (init_client(s_server_ip, s_server_port)) {
                s_client_connected = true;
                send_initial_state(get_client_socket_fd());
            } else {
                usleep(100000);
            }
        }

        usleep(50000);
    }

    return NULL;
}

static void *receive_thread_func(void *arg) {
    (void) arg;

    char recv_buf[64];
    while (s_running && !is_shutdown_requested()) {
        if (!s_client_connected) {
            usleep(100000);
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
            // Ensure null termination
            recv_buf[ret] = '\0';

            // Split messages by newline in case multiple came in
            char *message = strtok(recv_buf, "\n");
            while (message != NULL) {
                if (strncmp(message, "HP:", 3) == 0) {
                    s_tank_health = atoi(message + 3);
                } else if (strcmp(message, "HIT") == 0) {
                    SoundEffects_playHit();
                    flash_LED(RED, 3, 333);
                } else if (strcmp(message, "GAME_OVER") == 0) {
                    SoundEffects_playLost();
                    printf("Received game over from server. Shutting down...\n");
                    request_shutdown();
                    break;
                }

                message = strtok(NULL, "\n");
            }
        }

        usleep(50000);
    }

    return NULL;
}


// Periodically display the tank's current health on the LCD.
static void *lcd_thread_func(void *arg) {
    (void) arg;
    while (s_running && !is_shutdown_requested()) {
        int localHealth = s_tank_health;
        DisplayTankStatus(localHealth);

        // Sleep half a second so we aren't spamming the LCD too often
        usleep(500000);
    }

    return NULL;
}


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
    SoundEffects_init();
    init_LEDs();

    if (!Accelerometer_init()) {
        fprintf(stderr, "Warning: Accelerometer init failed. Cheat code will be unavailable.\n");
    }

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

    if (pthread_create(&s_accelerometer_thread, NULL, accelerometer_thread_func, NULL) != 0) {
        perror("Failed to create accelerometer thread");
        goto error_cleanup_lcd;
    }

    return true;

    error_cleanup_lcd:
    pthread_cancel(s_lcd_thread);
    pthread_join(s_lcd_thread, NULL);

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
    cleanup_LEDs();
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
    pthread_join(s_receive_thread, NULL);
    pthread_join(s_lcd_thread, NULL);
    pthread_join(s_accelerometer_thread, NULL);

    // Cleanup modules
    cleanup_joystick();
    RotaryEncoder_cleanup();
    Gpio_cleanup();
    turnOffLCD();
    DrawStuff_cleanup();
    SoundEffects_cleanup();
    cleanup_client();
    cleanup_LEDs();
    Accelerometer_cleanup();
}
