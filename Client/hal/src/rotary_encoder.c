#include "../include/rotary_encoder.h"
#include "../include/gpio.h"
#include <stdbool.h>
#include <assert.h>

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
static int s_rotationDelta = 0;
static bool s_buttonPressed = false;
static bool s_buttonStateChanged = false;

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

    // Open lines for input
    s_lineA = Gpio_openForEvents(ENCODER_CHIP, PIN_A);
    s_lineB = Gpio_openForEvents(ENCODER_CHIP, PIN_B);
    s_lineButton = Gpio_openForEvents(GPIO_CHIP_0, BUTTON_PIN);

    // Initialize state variables
    s_rotationDelta = 0;
    s_buttonPressed = false;
    s_buttonStateChanged = false;
}

void RotaryEncoder_cleanup(void) {
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

int RotaryEncoder_readRotation(void) {
    // We'll use the event-based approach since your GPIO module is optimized for it
    struct gpiod_line_bulk bulkEvents;
    int rotation = 0;

    // Check for changes on both A and B lines
    int numEvents = Gpio_waitForLineChange(s_lineA, &bulkEvents);
    if (numEvents > 0) {
        // Process state changes for line A
        int a_val = gpiod_line_get_value((struct gpiod_line*)s_lineA);

        if (a_val) {
            if (pCurrentState->a_rising.action) {
                pCurrentState->a_rising.action();
            }
            pCurrentState = pCurrentState->a_rising.pNextState;
        } else {
            if (pCurrentState->a_falling.action) {
                pCurrentState->a_falling.action();
            }
            pCurrentState = pCurrentState->a_falling.pNextState;
        }
    }

    numEvents = Gpio_waitForLineChange(s_lineB, &bulkEvents);
    if (numEvents > 0) {
        // Process state changes for line B
        int b_val = gpiod_line_get_value((struct gpiod_line*)s_lineB);

        if (b_val) {
            if (pCurrentState->b_rising.action) {
                pCurrentState->b_rising.action();
            }
            pCurrentState = pCurrentState->b_rising.pNextState;
        } else {
            if (pCurrentState->b_falling.action) {
                pCurrentState->b_falling.action();
            }
            pCurrentState = pCurrentState->b_falling.pNextState;
        }
    }

    // Return and reset accumulated rotation
    rotation = s_rotationDelta;
    s_rotationDelta = 0;
    return rotation;
}

bool RotaryEncoder_readButton(void) {
    struct gpiod_line_bulk bulkEvents;
    bool buttonPressed = false;

    // Check for button changes
    int numEvents = Gpio_waitForLineChange(s_lineButton, &bulkEvents);
    if (numEvents > 0) {
        int button_val = gpiod_line_get_value((struct gpiod_line*)s_lineButton);
        bool currentState = (button_val == 0); // Assuming active-low button

        if (currentState && !s_buttonPressed) {
            buttonPressed = true;
        }

        s_buttonPressed = currentState;
    }

    return buttonPressed;
}