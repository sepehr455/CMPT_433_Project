#ifndef REACTIONTIMER_JOYSTICKMODULE_H
#define REACTIONTIMER_JOYSTICKMODULE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * A module for reading the joystick on the beagle bone
 */

typedef enum {
    NO_DIRECTION,
    UP,
    DOWN,
    LEFT,
    RIGHT,
} JoystickDirection;

typedef struct {
    JoystickDirection direction;
    int16_t x;
    int16_t y;
} JoystickOutput;

void init_joystick(void);
void cleanup_joystick(void);
JoystickOutput read_joystick(void);



#endif
