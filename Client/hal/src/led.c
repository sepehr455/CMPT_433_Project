#include "../include/led.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define LED_GREEN_PATH "/sys/class/leds/ACT/brightness"
#define LED_RED_PATH "/sys/class/leds/PWR/brightness"

#define LED_GREEN_TRIGGER "/sys/class/leds/ACT/trigger"
#define LED_RED_TRIGGER "/sys/class/leds/PWR/trigger"

#define LED_COUNT (sizeof(LEDs) / sizeof(LEDs[0]))
#define ASSERT_LED_INITIALIZED() assert(is_initialized && "LED module must be initialized first!")


static bool is_initialized = false;


static LED LEDs[] = {
        {GREEN, false},
        {RED,   false}
};


static const char *LED_BRIGHTNESS_PATHS[] = {
        LED_GREEN_PATH,
        LED_RED_PATH
};

static const char *LED_TRIGGER_PATHS[] = {
        LED_GREEN_TRIGGER,
        LED_RED_TRIGGER
};


// A simple helper function to write a value to a file
static void write_to_file(const char *file_path, const char *value) {
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error opening LED file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", value);
    fclose(file);
}

// Get the LED struct for a given color
static LED* get_LED(LEDColor color) {
    for (size_t i = 0; i < LED_COUNT; i++) {
        if (LEDs[i].color == color) {
            return &LEDs[i];
        }
    }
    return NULL;
}

void init_LEDs(void) {
    if (!is_initialized) {

        // Remove the default triggers for the LEDs
        for (size_t i = 0; i < LED_COUNT; i++) {
            write_to_file(LED_TRIGGER_PATHS[i], "none");
            LEDs[i].is_on = false;
        }
        is_initialized = true;
    }
}

void cleanup_LEDs(void) {
    if (is_initialized) {
        turn_LED_off(GREEN);
        turn_LED_off(RED);

        // Restore the default triggers for the LEDs
        write_to_file(LED_GREEN_TRIGGER, "none");
        write_to_file(LED_RED_TRIGGER, "none");


        is_initialized = false;
    }
}

void turn_LED_on(LEDColor color) {
    ASSERT_LED_INITIALIZED();
    LED *led = get_LED(color);
    if (led) {
        write_to_file(LED_BRIGHTNESS_PATHS[color], "1");
        led->is_on = true;
    }
}

void turn_LED_off(LEDColor color) {
    ASSERT_LED_INITIALIZED();
    LED *led = get_LED(color);
    if (led) {
        write_to_file(LED_BRIGHTNESS_PATHS[color], "0");
        led->is_on = false;
    }
}

void flash_LED(LEDColor color, int flashes, int duration_ms) {
    ASSERT_LED_INITIALIZED();
    LED *led = get_LED(color);
    if (!led) return;

    for (int i = 0; i < flashes; i++) {
        turn_LED_on(color);
        usleep((duration_ms / 2) * 1000);
        turn_LED_off(color);
        usleep((duration_ms / 2) * 1000);
    }
}
