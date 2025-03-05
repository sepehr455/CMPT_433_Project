#include "../include/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// I2C Configuration
#define I2C_BUS "/dev/i2c-1"

// Register where the ADC data is stored
#define REG_DATA 0x00

// ADC Configuration for X and Y channels respectively
#define TLA2024_CHANNEL_CONF_X 0x83D2
#define TLA2024_CHANNEL_CONF_Y 0x83C2

static int I2C_fd = -1;

// Initialize I2C Bus
static int init_I2C_bus(const char *bus) {
    int fd = open(bus, O_RDWR);
    if (fd == -1) {
        perror("I2C: Failed to open bus");
        exit(EXIT_FAILURE);
    }

    // 72 is the address of the ADC
    if (ioctl(fd, I2C_SLAVE, 72) == -1) {
        perror("I2C: Failed to set device address");
        close(fd);
        exit(EXIT_FAILURE);
    }
    return fd;
}

// Write to an I2C register
static void write_I2C_reg16(int fd, uint16_t value) {
    uint8_t buffer[3];


    buffer[0] = 1;

    // Split 16-bit value into two 8-bit values
    buffer[1] = (value & 0xFF);
    buffer[2] = (value >> 8) & 0xFF;

    if (write(fd, buffer, 3) != 3) {
        perror("I2C: Failed to write register");
        exit(EXIT_FAILURE);
    }
}

// Read a 16-bit register value
static uint16_t read_I2C_reg16(int fd, uint8_t reg_addr) {
    if (write(fd, &reg_addr, 1) != 1) {
        perror("I2C: Failed to set read register");
        exit(EXIT_FAILURE);
    }

    uint8_t buffer[2];
    if (read(fd, buffer, 2) != 2) {
        perror("I2C: Failed to read register");
        exit(EXIT_FAILURE);
    }

    // Combine bytes (the ADC is 12-bit)
    return ((buffer[0] << 8) | buffer[1]) >> 4;
}

// **Initialize Joystick**
void init_joystick(void) {
    if (I2C_fd == -1) {
        I2C_fd = init_I2C_bus(I2C_BUS);
    }

    // Configure ADC channels
    write_I2C_reg16(I2C_fd, TLA2024_CHANNEL_CONF_X);
    usleep(10000);
    write_I2C_reg16(I2C_fd, TLA2024_CHANNEL_CONF_Y);
    usleep(10000);
}

// **Cleanup Joystick**
void cleanup_joystick(void) {
    if (I2C_fd != -1) {
        close(I2C_fd);
        I2C_fd = -1;
    }
}

// **Read Joystick and Determine Direction**
JoystickOutput read_joystick(void) {
    JoystickOutput output = {NO_DIRECTION, 0, 0};

    // Case where joystick is not initialized
    if (I2C_fd == -1) {
        fprintf(stderr, "Joystick module not initialized!\n");
        return output;
    }

    // Read raw X and Y values
    write_I2C_reg16(I2C_fd, TLA2024_CHANNEL_CONF_X);
    usleep(10000);
    uint16_t raw_x = read_I2C_reg16(I2C_fd, REG_DATA);

    write_I2C_reg16(I2C_fd, TLA2024_CHANNEL_CONF_Y);
    usleep(10000);
    uint16_t raw_y = read_I2C_reg16(I2C_fd, REG_DATA);


    // Constants for joystick calibration
    const int X_NEUTRAL = 833;
    const int Y_NEUTRAL = 862;
    const int X_MAX = 1400;
    const int X_MIN = 400;
    const int Y_MAX = 1400;
    const int Y_MIN = 300;


    // Normalize raw values to -100 to 100
    int normalized_x = ((raw_x - X_NEUTRAL) * 100) /
                       ((raw_x > X_NEUTRAL) ? (X_MAX - X_NEUTRAL) : (X_NEUTRAL - X_MIN));

    int normalized_y = ((Y_NEUTRAL - raw_y) * 100) /
                       ((raw_y < Y_NEUTRAL) ? (Y_NEUTRAL - Y_MIN) : (Y_MAX - Y_NEUTRAL));


    // Clamp values to ensure they stay within -100 to 100
    if (normalized_x > 100) normalized_x = 100;
    if (normalized_x < -100) normalized_x = -100;
    if (normalized_y > 100) normalized_y = 100;
    if (normalized_y < -100) normalized_y = -100;

    // Assign normalized values to output struct
    output.x = (int16_t) normalized_x;
    output.y = (int16_t) normalized_y;

    // Determine direction based on normalized values
    if (output.y > 50) {
        output.direction = UP;
    } else if (output.y < -50) {
        output.direction = DOWN;
    } else if (output.x > 50) {
        output.direction = RIGHT;
    } else if (output.x < -50) {
        output.direction = LEFT;
    }

    return output;
}
