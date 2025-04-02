#include "../include/draw_stuff.h"

#include "../../lcd/lib/Config/DEV_Config.h"
#include "../../lcd/lib/LCD/LCD_1in54.h"
#include "../../lcd/lib/GUI/GUI_Paint.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>


// LCD frame buffer
static UWORD *s_fb = NULL;
static bool s_isInitialized = false;

void DrawStuff_init(void) {
    assert(!s_isInitialized);

    // Initialize hardware
    if (DEV_ModuleInit() != 0) {
        DEV_ModuleExit();
        exit(1);
    }

    // Initialize LCD
    DEV_Delay_ms(2000);
    LCD_1IN54_Init(HORIZONTAL);
    LCD_1IN54_Clear(WHITE);
    LCD_SetBacklight(1023);

    // Allocate frame buffer
    UDOUBLE imagesize = LCD_1IN54_HEIGHT * LCD_1IN54_WIDTH * 2;
    s_fb = (UWORD *) malloc(imagesize);
    if (!s_fb) {
        perror("Failed to allocate LCD frame buffer");
        DEV_ModuleExit();
        exit(1);
    }

    s_isInitialized = true;
}



void DrawStuff_cleanup(void) {
    assert(s_isInitialized);

    free(s_fb);
    s_fb = NULL;

    DEV_ModuleExit();
    s_isInitialized = false;
}


void turnOffLCD(void) {
    assert(s_isInitialized);

    LCD_1IN54_Clear(WHITE);
    DEV_Delay_ms(1000);
    LCD_SetBacklight(0);
}


// Screen 1: Beat Status Screen
// Displays the beat name (center), volume (bottom left), and BPM (bottom right)
void DisplayScreen1(const char* beatName, int volume, int bpm) {
    assert(s_isInitialized);

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Display the beat name in the middle of the screen.
    int centerX = 5;
    int centerY = (LCD_1IN54_HEIGHT / 2) - 10;
    Paint_DrawString_EN(centerX, centerY, (char *)beatName, &Font16, WHITE, BLACK);

    // Display volume at bottom left
    char line[64];
    snprintf(line, sizeof(line), "Vol: %d", volume);
    Paint_DrawString_EN(5, LCD_1IN54_HEIGHT - 20, line, &Font16, WHITE, BLACK);

    // Display BPM at bottom right
    snprintf(line, sizeof(line), "%d BPM", bpm);

    // Calculate the width of the BPM text
    int textWidth = strlen(line) * Font16.Width;

    // Position the BPM text at the bottom right, with a small margin
    int bpmX = LCD_1IN54_WIDTH - textWidth - 5;
    if (bpmX < 0) bpmX = 0;  // Prevent out-of-bounds text placement

    int bpmY = LCD_1IN54_HEIGHT - 20;


    Paint_DrawString_EN(bpmX, bpmY, line, &Font16, WHITE, BLACK);

    LCD_1IN54_Display(s_fb);
}

// Screen 2: Audio Timing Screen
// Displays a title and sample timing values (min, max, avg)
void DisplayScreen2(int audioMin, int audioMax, double audioAvg) {
    assert(s_isInitialized);

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Title
    Paint_DrawString_EN(5, 5, "Audio Timing", &Font16, WHITE, BLACK);

    // Display timing info
    char line[64];
    snprintf(line, sizeof(line), "Min: %d ms", audioMin);
    Paint_DrawString_EN(5, 30, line, &Font16, WHITE, BLACK);

    snprintf(line, sizeof(line), "Max: %d ms", audioMax);
    Paint_DrawString_EN(5, 55, line, &Font16, WHITE, BLACK);

    snprintf(line, sizeof(line), "Avg: %.2f ms", audioAvg);
    Paint_DrawString_EN(5, 80, line, &Font16, WHITE, BLACK);

    LCD_1IN54_Display(s_fb);
}

// Screen 3: Accelerometer Timing Screen
// Displays a title and sample timing values for the accelerometer (min, max, avg)
void DisplayScreen3(int accelMin, int accelMax, double accelAvg) {
    assert(s_isInitialized);

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Title
    Paint_DrawString_EN(5, 5, "Accel. Timing", &Font16, WHITE, BLACK);

    // Display timing info
    char line[64];
    snprintf(line, sizeof(line), "Min: %d ms", accelMin);
    Paint_DrawString_EN(5, 30, line, &Font16, WHITE, BLACK);

    snprintf(line, sizeof(line), "Max: %d ms", accelMax);
    Paint_DrawString_EN(5, 55, line, &Font16, WHITE, BLACK);

    snprintf(line, sizeof(line), "Avg: %.2f ms", accelAvg);
    Paint_DrawString_EN(5, 80, line, &Font16, WHITE, BLACK);

    LCD_1IN54_Display(s_fb);
}
