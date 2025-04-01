#include "../include/gpio.h"
#include <stdlib.h>
#include <stdio.h>
#include <gpiod.h>
#include <assert.h>
#include <errno.h>

// Relies on the gpiod library.
// Insallation for cross compiling:
//      (host)$ sudo dpkg --add-architecture arm64
//      (host)$ sudo apt update
//      (host)$ sudo apt install libgpdiod-dev:arm64
// GPIO: https://www.ics.com/blog/gpio-programming-exploring-libgpiod-library
// Example: https://github.com/starnight/libgpiod-example/blob/master/libgpiod-input/main.c

static bool s_isInitialized = false;

static char* s_chipNames[] = {
        "gpiochip0",
        "gpiochip1",
        "gpiochip2",
};

// Hold open chips
static struct gpiod_chip* s_openGpiodChips[GPIO_NUM_CHIPS];

void Gpio_initialize(void)
{
    for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
        // Open GPIO chip
        s_openGpiodChips[i] = gpiod_chip_open_by_name(s_chipNames[i]);
        if (!s_openGpiodChips[i]) {
            perror("GPIO Initializing: Unable to open GPIO chip");
            exit(EXIT_FAILURE);
        }
    }
    s_isInitialized = true;
}

void Gpio_cleanup(void)
{
    assert(s_isInitialized);
    for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
        if (s_openGpiodChips[i]) {
            gpiod_chip_close(s_openGpiodChips[i]);
            s_openGpiodChips[i] = NULL;
        }
    }
    s_isInitialized = false;
}

struct GpioLine* Gpio_openForEvents(enum eGpioChips chip, int pinNumber)
{
    assert(s_isInitialized);
    struct gpiod_chip* gpiodChip = s_openGpiodChips[chip];
    struct gpiod_line* line = gpiod_chip_get_line(gpiodChip, pinNumber);
    if (!line) {
        perror("Unable to get GPIO line");
        exit(EXIT_FAILURE);
    }

    return (struct GpioLine*) line;
}

void Gpio_close(struct GpioLine* line)
{
    assert(s_isInitialized);
    gpiod_line_release((struct gpiod_line*) line);
}

void Gpio_requestEdgeEvents(struct GpioLine* line, enum eGpioEdge edge)
{
    assert(s_isInitialized);
    struct gpiod_line* gpiodLine = (struct gpiod_line*) line;

    int ret;
    switch (edge) {
        case GPIO_EDGE_RISING:
            ret = gpiod_line_request_rising_edge_events(gpiodLine, "EdgeEvents");
            break;
        case GPIO_EDGE_FALLING:
            ret = gpiod_line_request_falling_edge_events(gpiodLine, "EdgeEvents");
            break;
        case GPIO_EDGE_BOTH:
            ret = gpiod_line_request_both_edges_events(gpiodLine, "EdgeEvents");
            break;
        default:
            return; // No edge detection requested
    }

    if (ret < 0) {
        perror("Failed to request edge events");
        exit(EXIT_FAILURE);
    }
}

int Gpio_waitForLineChange(struct GpioLine* line1, struct gpiod_line_bulk *bulkEvents)
{
    assert(s_isInitialized);

    struct gpiod_line_bulk bulkWait;
    gpiod_line_bulk_init(&bulkWait);
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line1);

    int result = gpiod_line_event_wait_bulk(&bulkWait, NULL, bulkEvents);
    if (result == -1) {
        perror("Error waiting on lines for event waiting");
        exit(EXIT_FAILURE);
    }

    return gpiod_line_bulk_num_lines(bulkEvents);
}

bool Gpio_checkForEvent(struct GpioLine* line, struct gpiod_line_bulk *bulkEvents)
{
    assert(s_isInitialized);

    struct gpiod_line_bulk bulkWait;
    gpiod_line_bulk_init(&bulkWait);
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*)line);

    // Use zero timeout for non-blocking check
    struct timespec timeout = {0, 0};
    int result = gpiod_line_event_wait_bulk(&bulkWait, &timeout, bulkEvents);

    if (result == -1) {
        if (errno != EAGAIN) { // Only report non-timeout errors
            perror("Error checking for events");
            exit(EXIT_FAILURE);
        }
        return false;
    }

    return (gpiod_line_bulk_num_lines(bulkEvents) > 0);
}