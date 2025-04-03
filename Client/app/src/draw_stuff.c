#include "../include/draw_stuff.h"

#include "../../lcd/lib/Config/DEV_Config.h"
#include "../../lcd/lib/LCD/LCD_1in54.h"
#include "../../lcd/lib/GUI/GUI_Paint.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

// If your library doesn't define these, you can define them here:
#ifndef RED
#define RED 0xF800
#endif
#ifndef GREEN
#define GREEN 0x07E0
#endif
#ifndef YELLOW
#define YELLOW 0xFFE0
#endif
#ifndef BLACK
#define BLACK 0x0000
#endif
#ifndef WHITE
#define WHITE 0xFFFF
#endif

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
    printf("LCD: Cleaning up LCD\n");
    assert(s_isInitialized);

    free(s_fb);
    s_fb = NULL;

    DEV_ModuleExit();
    s_isInitialized = false;
}


void turnOffLCD(void) {
    printf("LCD: Turning off LCD\n");
    assert(s_isInitialized);

    LCD_1IN54_Clear(WHITE);
    DEV_Delay_ms(1000);
    LCD_SetBacklight(0);
}



//  Display the tank's health in four states.
//
#ifndef ORANGE
#define ORANGE 0xFD20
#endif
#ifndef GRAY
#define GRAY 0x8410  // Mid-tone gray in RGB565
#endif
#ifndef DARK_GRAY
#define DARK_GRAY 0x4208
#endif

void DisplayTankStatus(int health) {
    assert(s_isInitialized);

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Draw health text
    char statusLine[64];
    if (health >= 3) sprintf(statusLine, "Tank Health: FULL (3/3)");
    else if (health == 2) sprintf(statusLine, "Tank Health: Damaged (2/3)");
    else if (health == 1) sprintf(statusLine, "Tank Health: Critical (1/3)");
    else sprintf(statusLine, "Tank Health: DESTROYED (0/3)");

    Paint_DrawString_EN(5, 5, statusLine, &Font16, WHITE, BLACK);

    // --- Tank Geometry ---
    // Wider tank shape (horizontal layout)
    int hullLeft = 60;
    int hullTop = 100;
    int hullRight = 180;
    int hullBottom = 140;

    int turretLeft = 100;
    int turretTop = 110;
    int turretRight = 140;
    int turretBottom = 130;

    int barrelStartX = turretRight;
    int barrelStartY = (turretTop + turretBottom) / 2;
    int barrelEndX = 200;
    int barrelEndY = barrelStartY;

    // Tank color based on health
    UWORD tankColor;
    if (health >= 3) tankColor = GREEN;
    else if (health == 2) tankColor = YELLOW;
    else if (health == 1) tankColor = RED;
    else tankColor = DARK_GRAY;

    // Draw hull
    Paint_DrawRectangle(hullLeft, hullTop, hullRight, hullBottom, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(hullLeft + 1, hullTop + 1, hullRight - 1, hullBottom - 1, tankColor, DOT_PIXEL_1X1,
                        DRAW_FILL_FULL);

    // Draw turret
    Paint_DrawRectangle(turretLeft, turretTop, turretRight, turretBottom, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(turretLeft + 1, turretTop + 1, turretRight - 1, turretBottom - 1, tankColor, DOT_PIXEL_1X1,
                        DRAW_FILL_FULL);

    // Draw barrel
    if (health > 0) {
        Paint_DrawLine(barrelStartX, barrelStartY, barrelEndX, barrelEndY, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    }

    // Damage effects
    if (health == 2) {
        Paint_DrawLine(70, 110, 80, 120, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawLine(150, 125, 160, 135, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    } else if (health == 1) {
        Paint_DrawLine(70, 110, 90, 125, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawLine(160, 105, 150, 115, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawLine(100, 135, 110, 145, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawLine(140, 120, 150, 130, RED, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

        // Small flame
        Paint_DrawCircle(150, 100, 6, ORANGE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(150, 98, 3, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else if (health <= 0) {
        // X across tank
        Paint_DrawLine(hullLeft, hullTop, hullRight, hullBottom, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
        Paint_DrawLine(hullRight, hullTop, hullLeft, hullBottom, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

        // Smoke effect
        Paint_DrawCircle(140, 90, 10, GRAY, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(150, 85, 8, DARK_GRAY, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(130, 95, 6, GRAY, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(145, 100, 5, GRAY, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Big flame
        Paint_DrawCircle(150, 105, 10, ORANGE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(150, 102, 5, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(153, 107, 3, YELLOW, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    LCD_1IN54_Display(s_fb);
}
