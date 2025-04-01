// Low-level GPIO access using gpiod
#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <gpiod.h>

/**
 * Module for low-level GPIO access using libgpiod.
 */

// Opaque structure
struct GpioLine;

enum eGpioChips {
    GPIO_CHIP_0,
    GPIO_CHIP_1,
    GPIO_CHIP_2,
    GPIO_NUM_CHIPS      // Count of chips
};

enum eGpioEdge {
    GPIO_EDGE_NONE,
    GPIO_EDGE_RISING,
    GPIO_EDGE_FALLING,
    GPIO_EDGE_BOTH
};

// Must initialize before calling any other functions.
void Gpio_initialize(void);
void Gpio_cleanup(void);

// Opening a pin gives us a "line" that we later work with.
struct GpioLine* Gpio_openForEvents(enum eGpioChips chip, int pinNumber);

// Event handling functions
int Gpio_waitForLineChange(struct GpioLine* line1, struct gpiod_line_bulk *bulkEvents);
bool Gpio_checkForEvent(struct GpioLine* line, struct gpiod_line_bulk *bulkEvents);
void Gpio_requestEdgeEvents(struct GpioLine* line, enum eGpioEdge edge);

// Utility functions
void Gpio_close(struct GpioLine* line);

#endif