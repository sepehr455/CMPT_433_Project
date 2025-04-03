#include <stdbool.h>

#ifndef LED_H
#define LED_H

/**
 * A module for controlling the LEDs on the beagle bone
 */

typedef enum {
    GREEN,
    RED
} LEDColor;

typedef struct {
    LEDColor color;
    bool is_on;
} LED;


void init_LEDs(void);
void cleanup_LEDs(void);

void turn_LED_on(LEDColor color);
void turn_LED_off(LEDColor color);
void flash_LED(LEDColor color, int flashes, int duration_ms);


#endif
