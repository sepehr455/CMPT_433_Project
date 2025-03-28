#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

#include <stdint.h>

/**
 * Module for reading a rotary encoder.
 * -
 */

// Initialize the rotary encoder
void RotaryEncoder_init(void);

// Cleanup the rotary encoder
void RotaryEncoder_cleanup(void);

// Get the current frequency (Hz)
int RotaryEncoder_getFrequency(void);

// Get the button press count
int RotaryEncoder_getButtonPressCount(void);

void RotaryEncoder_poll(void);

#endif // _ROTARY_ENCODER_H_