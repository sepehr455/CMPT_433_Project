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

// Display the tank's health (0..3).
// 3 -> fully healthy, 2 -> lightly damaged, 1 -> heavily damaged, 0 -> destroyed
void DisplayTankStatus(int health);

#endif