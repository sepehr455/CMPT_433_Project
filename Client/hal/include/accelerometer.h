#ifndef _ACCELEROMETER_H_
#define _ACCELEROMETER_H_

#include <stdbool.h>

// Initialize I2C and accelerometer sensor
bool Accelerometer_init(void);

// Read raw X, Y, Z accelerometer values
void Accelerometer_readRaw(float *x, float *y, float *z);

// Read tilt direction
void Accelerometer_getTiltDirection(float *x_tilt, float *y_tilt);

// Cleanup resources
void Accelerometer_cleanup(void);

#endif
