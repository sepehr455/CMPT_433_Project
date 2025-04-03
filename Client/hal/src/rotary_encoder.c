#include "../include/rotary_encoder.h"
#include "../include/gpio.h"
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

#define ENCODER_CHIP  GPIO_CHIP_2
#define PIN_A         7
#define PIN_B         8
#define BUTTON_PIN    10

static struct GpioLine *s_lineA = NULL;
static struct GpioLine *s_lineB = NULL;
static struct GpioLine *s_lineButton = NULL;

// Rotary Encoder State Machine
typedef void (*ActionFn)(void);

struct stateEvent {
    struct state *pNextState;
    ActionFn action;
};

struct state {
    struct stateEvent a_rising;
    struct stateEvent a_falling;
    struct stateEvent b_rising;
    struct stateEvent b_falling;
};

enum {
    ST_REST = 0, ST_1, ST_2, ST_3, NUM_STATES
};

static struct state states[NUM_STATES];
static struct state *pCurrentState = &states[ST_REST];
static volatile int s_rotationDelta = 0;

// Button state variables
static volatile bool s_buttonPressed = false;
static volatile bool s_buttonStateChanged = false;
static volatile bool s_buttonHandled = true;

// Debouncing variables
static struct timespec s_lastAEvent = {0, 0};
static struct timespec s_lastBEvent = {0, 0};
static struct timespec s_lastButtonEvent = {0, 0};
#define DEBOUNCE_NS 5000000  // 5ms debounce time

// Transition functions for rotary encoder
static void onClockwise(void) {
    s_rotationDelta++;
}

static void onCounterClockwise(void) {
    s_rotationDelta--;
}

static void doNothing(void) {}

static void initStates(void) {
    // REST state
    states[ST_REST].a_rising = (struct stateEvent) {&states[ST_REST], doNothing};
    states[ST_REST].a_falling = (struct stateEvent) {&states[ST_1], doNothing};
    states[ST_REST].b_rising = (struct stateEvent) {&states[ST_REST], doNothing};
    states[ST_REST].b_falling = (struct stateEvent) {&states[ST_3], doNothing};

    // S1
    states[ST_1].a_rising = (struct stateEvent) {&states[ST_REST], onCounterClockwise};
    states[ST_1].a_falling = (struct stateEvent) {&states[ST_1], doNothing};
    states[ST_1].b_rising = (struct stateEvent) {&states[ST_1], doNothing};
    states[ST_1].b_falling = (struct stateEvent) {&states[ST_2], doNothing};

    // S2
    states[ST_2].a_rising = (struct stateEvent) {&states[ST_3], doNothing};
    states[ST_2].a_falling = (struct stateEvent) {&states[ST_2], doNothing};
    states[ST_2].b_rising = (struct stateEvent) {&states[ST_1], doNothing};
    states[ST_2].b_falling = (struct stateEvent) {&states[ST_2], doNothing};

    // S3
    states[ST_3].a_rising = (struct stateEvent) {&states[ST_3], doNothing};
    states[ST_3].a_falling = (struct stateEvent) {&states[ST_2], doNothing};
    states[ST_3].b_rising = (struct stateEvent) {&states[ST_REST], onClockwise};
    states[ST_3].b_falling = (struct stateEvent) {&states[ST_3], doNothing};
}

void RotaryEncoder_init(void) {
    // Build the state table once
    initStates();
    pCurrentState = &states[ST_REST];

    // Open lines for input with edge detection
    s_lineA = Gpio_openForEvents(ENCODER_CHIP, PIN_A);
    s_lineB = Gpio_openForEvents(ENCODER_CHIP, PIN_B);
    s_lineButton = Gpio_openForEvents(GPIO_CHIP_0, BUTTON_PIN);

    // Request both rising and falling edge detection
    Gpio_requestEdgeEvents(s_lineA, GPIO_EDGE_BOTH);
    Gpio_requestEdgeEvents(s_lineB, GPIO_EDGE_BOTH);
    Gpio_requestEdgeEvents(s_lineButton, GPIO_EDGE_BOTH);

    // Initialize state variables
    s_rotationDelta = 0;
    s_buttonPressed = false;
    s_buttonStateChanged = false;
}

void RotaryEncoder_cleanup(void) {
    fprintf(stderr, "Cleaning up rotary encoder...\n");
    if (s_lineA) {
        Gpio_close(s_lineA);
        s_lineA = NULL;
    }
    if (s_lineB) {
        Gpio_close(s_lineB);
        s_lineB = NULL;
    }
    if (s_lineButton) {
        Gpio_close(s_lineButton);
        s_lineButton = NULL;
    }
}

void RotaryEncoder_processEvents(void) {
    struct gpiod_line_bulk bulkEvents;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Check for changes on line A
    if (Gpio_checkForEvent(s_lineA, &bulkEvents)) {
        long time_since_last = (now.tv_sec - s_lastAEvent.tv_sec) * 1000000000L +
                               (now.tv_nsec - s_lastAEvent.tv_nsec);

        if (time_since_last > DEBOUNCE_NS) {
            int a_val = gpiod_line_get_value((struct gpiod_line*)s_lineA);
            if (a_val) {
                if (pCurrentState->a_rising.action) pCurrentState->a_rising.action();
                pCurrentState = pCurrentState->a_rising.pNextState;
            } else {
                if (pCurrentState->a_falling.action) pCurrentState->a_falling.action();
                pCurrentState = pCurrentState->a_falling.pNextState;
            }
            s_lastAEvent = now;
        }
    }

    // Check for changes on line B
    if (Gpio_checkForEvent(s_lineB, &bulkEvents)) {
        long time_since_last = (now.tv_sec - s_lastBEvent.tv_sec) * 1000000000L +
                               (now.tv_nsec - s_lastBEvent.tv_nsec);

        if (time_since_last > DEBOUNCE_NS) {
            int b_val = gpiod_line_get_value((struct gpiod_line*)s_lineB);
            if (b_val) {
                if (pCurrentState->b_rising.action) pCurrentState->b_rising.action();
                pCurrentState = pCurrentState->b_rising.pNextState;
            } else {
                if (pCurrentState->b_falling.action) pCurrentState->b_falling.action();
                pCurrentState = pCurrentState->b_falling.pNextState;
            }
            s_lastBEvent = now;
        }
    }

    // Check for button changes
    if (Gpio_checkForEvent(s_lineButton, &bulkEvents)) {
        long time_since_last = (now.tv_sec - s_lastButtonEvent.tv_sec) * 1000000000L +
                              (now.tv_nsec - s_lastButtonEvent.tv_nsec);

        if (time_since_last > DEBOUNCE_NS) {
            int button_val = gpiod_line_get_value((struct gpiod_line*)s_lineButton);
            bool currentState = (button_val == 0); // Assuming active-low button

            // Only register new presses if button was released since last press
            if (currentState && (!s_buttonPressed || s_buttonHandled)) {
                s_buttonPressed = true;
                s_buttonStateChanged = true;
                s_buttonHandled = false;
            } else if (!currentState) {
                s_buttonPressed = false;
                s_buttonHandled = true;
            }

            s_lastButtonEvent = now;
        }
    }
}

int RotaryEncoder_readRotation(void) {
    int rotation = s_rotationDelta;
    s_rotationDelta = 0;
    return rotation;
}

bool RotaryEncoder_readButton(void) {
    if (s_buttonStateChanged && s_buttonPressed) {
        s_buttonStateChanged = false;
        return true;
    }
    return false;
}