#ifndef _DRAW_STUFF_H_
#define _DRAW_STUFF_H_

/**
 * Module for drawing on the LCD screen.
 *
 * In addition to the existing update function, we add three new display functions
 * for each of the three screens required by the assignment.
 */

void DrawStuff_init(void);
void DrawStuff_cleanup(void);
void turnOffLCD(void);

// Screen 1: Displays the current beat name in large font, volume (bottom left) and BPM (bottom right)
void DisplayScreen1(const char* beatName, int volume, int bpm);

// Screen 2: Displays audio timing statistics (hardcoded for now)
void DisplayScreen2(int audioMin, int audioMax, double audioAvg);

// Screen 3: Displays accelerometer timing statistics (hardcoded for now)
void DisplayScreen3(int accelMin, int accelMax, double accelAvg);

// Display the tank's health (0..3).
// 3 -> fully healthy, 2 -> lightly damaged, 1 -> heavily damaged, 0 -> destroyed
void DisplayTankStatus(int health);

#endif