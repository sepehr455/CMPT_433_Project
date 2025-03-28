#include "../include/rotary_encoder.h"
#include "../include/gpio.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <gpiod.h>
#include <assert.h>
#include <time.h>

// Choose which GPIO chip and line offsets to open:
#define ENCODER_CHIP  GPIO_CHIP_2
#define PIN_A         7
#define PIN_B         8
#define BUTTON_PIN    10

static struct GpioLine *s_lineA = NULL;
static struct GpioLine *s_lineB = NULL;
static struct GpioLine *s_lineButton = NULL;

static atomic_int s_counter = 0;
static atomic_int s_buttonPressCount = 0;

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS 50

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

// Button State Machine
struct buttonStateEvent {
    struct buttonState *pNextState;
    void (*action)(void);
};

struct buttonState {
    struct buttonStateEvent rising;
    struct buttonStateEvent falling;
};

static struct buttonState buttonStates[] = {
        { // Not pressed
                .rising = {&buttonStates[0], NULL},
                .falling = {&buttonStates[1], NULL},
        },
        { // Pressed
                .rising = {&buttonStates[0], NULL},
                .falling = {&buttonStates[1], NULL},
        }
};

static struct buttonState *pCurrentButtonState = &buttonStates[0];

// Transition functions for rotary encoder
static void onClockwise(void) {
    atomic_fetch_add(&s_counter, 1);
}

static void onCounterClockwise(void) {
    atomic_fetch_sub(&s_counter, 1);
}

static void doNothing(void) {}

static void onButtonPress(void) {
    atomic_fetch_add(&s_buttonPressCount, 1);
}

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

    // Button states
    buttonStates[0].rising.action = NULL;
    buttonStates[0].falling.action = NULL;
    buttonStates[1].rising.action = onButtonPress;
    buttonStates[1].falling.action = NULL;
}

static void handle_encoder_event(struct gpiod_line_event *event, unsigned int offset) {
    bool isRising = (event->event_type == GPIOD_LINE_EVENT_RISING_EDGE);

    if (offset == PIN_A) {
        struct stateEvent *pTrans = isRising ? &pCurrentState->a_rising : &pCurrentState->a_falling;
        if (pTrans->action) pTrans->action();
        pCurrentState = pTrans->pNextState;
    } else if (offset == PIN_B) {
        struct stateEvent *pTrans = isRising ? &pCurrentState->b_rising : &pCurrentState->b_falling;
        if (pTrans->action) pTrans->action();
        pCurrentState = pTrans->pNextState;
    }
}

static void handle_button_event(struct gpiod_line_event *event) {
    bool isRising = (event->event_type == GPIOD_LINE_EVENT_RISING_EDGE);
    struct buttonStateEvent *pStateEvent = isRising ? &pCurrentButtonState->rising : &pCurrentButtonState->falling;

    if (pStateEvent->action) pStateEvent->action();
    pCurrentButtonState = pStateEvent->pNextState;
}

void RotaryEncoder_poll(void) {
    struct gpiod_line_bulk bulkEvents;
    gpiod_line_bulk_init(&bulkEvents);

    // Check encoder lines
    if (s_lineA && s_lineB) {
        struct gpiod_line_bulk bulkWait;
        gpiod_line_bulk_init(&bulkWait);
        gpiod_line_bulk_add(&bulkWait, (struct gpiod_line *)s_lineA);
        gpiod_line_bulk_add(&bulkWait, (struct gpiod_line *)s_lineB);

        if (gpiod_line_event_wait_bulk(&bulkWait, NULL, &bulkEvents) > 0) {
            int num = gpiod_line_bulk_num_lines(&bulkEvents);
            for (int i = 0; i < num; i++) {
                struct gpiod_line *line = gpiod_line_bulk_get_line(&bulkEvents, i);
                struct gpiod_line_event event;
                if (gpiod_line_event_read(line, &event) == 0) {
                    handle_encoder_event(&event, gpiod_line_offset(line));
                }
            }
        }
    }

    // Check button line
    if (s_lineButton) {
        int numEvents = Gpio_waitForLineChange(s_lineButton, &bulkEvents);
        if (numEvents > 0) {
            for (int i = 0; i < numEvents; i++) {
                struct gpiod_line *line = gpiod_line_bulk_get_line(&bulkEvents, i);
                struct gpiod_line_event event;
                if (gpiod_line_event_read(line, &event) == 0) {
                    handle_button_event(&event);

                    // Debounce delay
                    struct timespec ts = {0, DEBOUNCE_TIME_MS * 1000000L};
                    nanosleep(&ts, NULL);
                }
            }
        }
    }
}

void RotaryEncoder_init(void) {
    // Build the state table once
    initStates();
    pCurrentState = &states[ST_REST];
    pCurrentButtonState = &buttonStates[0];

    // Open lines
    s_lineA = Gpio_openForEvents(ENCODER_CHIP, PIN_A);
    s_lineB = Gpio_openForEvents(ENCODER_CHIP, PIN_B);
    s_lineButton = Gpio_openForEvents(GPIO_CHIP_0, BUTTON_PIN);

    // Initialize atomic counters
    atomic_store(&s_counter, 0);
    atomic_store(&s_buttonPressCount, 0);
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

int RotaryEncoder_getFrequency(void) {
    return atomic_load(&s_counter);
}

int RotaryEncoder_getButtonPressCount(void) {
    return atomic_load(&s_buttonPressCount);
}