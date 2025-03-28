#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Module for reading a rotary encoder.
 * -
 */

// Initialize the rotary encoder hardware
void RotaryEncoder_init(void);

// Cleanup the rotary encoder hardware
void RotaryEncoder_cleanup(void);

// Get the current rotation delta (call this periodically)
int RotaryEncoder_readRotation(void);

// Get the button press state (call this periodically)
bool RotaryEncoder_readButton(void);

#endif // _ROTARY_ENCODER_H_