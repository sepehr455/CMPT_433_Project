#include "../include/accelerometer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <math.h>

// I2C Configuration
#define I2C_BUS "/dev/i2c-1"
#define ACCELEROMETER_ADDR 0x19

// Accelerometer Registers
#define REG_WHO_AM_I 0x0F
#define REG_CTRL1    0x20
#define REG_OUT_X_L  0x28
#define REG_OUT_X_H  0x29
#define REG_OUT_Y_L  0x2A
#define REG_OUT_Y_H  0x2B
#define REG_OUT_Z_L  0x2C
#define REG_OUT_Z_H  0x2D

// Accelerometer Sensitivity (2g range)
#define SENSITIVITY  0.004f

static int i2c_fd = -1;

// Internal Helpers
static int init_i2c_bus(const char *bus, int address) {
    int fd = open(bus, O_RDWR);
    if (fd == -1) {
        perror("I2C: Failed to open bus");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, address) == -1) {
        perror("I2C: Failed to set device address");
        close(fd);
        return -1;
    }
    return fd;
}

// Write to an I2C register
static void write_i2c_reg(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (write(i2c_fd, buffer, 2) != 2) {
        perror("I2C: Failed to write register");
        exit(EXIT_FAILURE);
    }
}

// Read from an I2C register
static uint8_t read_i2c_reg(uint8_t reg) {
    if (write(i2c_fd, &reg, 1) != 1) {
        perror("I2C: Failed to set read register");
        exit(EXIT_FAILURE);
    }

    uint8_t value = 0;
    if (read(i2c_fd, &value, 1) != 1) {
        perror("I2C: Failed to read register");
        exit(EXIT_FAILURE);
    }
    return value;
}

// Read 16-bit value from two registers (LSB and MSB)
static int16_t read_i2c_reg16(uint8_t reg_lsb, uint8_t reg_msb) {
    int16_t lsb = read_i2c_reg(reg_lsb);
    int16_t msb = read_i2c_reg(reg_msb);
    return (int16_t)((msb << 8) | lsb);
}

// Public API
bool Accelerometer_init(void) {
    i2c_fd = init_i2c_bus(I2C_BUS, ACCELEROMETER_ADDR);
    if (i2c_fd == -1) {
        return false;
    }

    uint8_t who_am_i = read_i2c_reg(REG_WHO_AM_I);
    if (who_am_i != 0x44) {
        fprintf(stderr, "Accelerometer: Unexpected WHO_AM_I value (0x%02X)\n", who_am_i);
        close(i2c_fd);
        return false;
    }

    // Enable accelerometer
    write_i2c_reg(REG_CTRL1, 0x50);
    printf("Accelerometer initialized.\n");
    return true;
}

void Accelerometer_readRaw(float *x, float *y, float *z) {
    int16_t x_raw = read_i2c_reg16(REG_OUT_X_L, REG_OUT_X_H);
    int16_t y_raw = read_i2c_reg16(REG_OUT_Y_L, REG_OUT_Y_H);
    int16_t z_raw = read_i2c_reg16(REG_OUT_Z_L, REG_OUT_Z_H);

    *x = x_raw * SENSITIVITY;
    *y = y_raw * SENSITIVITY;
    *z = z_raw * SENSITIVITY;
}

void Accelerometer_getTiltDirection(float *x_tilt, float *y_tilt) {
    float x, y, z;
    Accelerometer_readRaw(&x, &y, &z);

    // Normalize based on the magnitude of gravity vector
    float magnitude = sqrt(x*x + y*y + z*z);
    if (magnitude == 0.0f) {
        *x_tilt = 0.0f;
        *y_tilt = 0.0f;
        return;
    }

    // Normalize and scale to [-1.0, +1.0] based on assignment spec
    *x_tilt = x / magnitude;
    *y_tilt = y / magnitude;
}

void Accelerometer_cleanup(void) {
    if (i2c_fd != -1) {
        close(i2c_fd);
        i2c_fd = -1;
    }
    printf("Accelerometer cleaned up.\n");
}
