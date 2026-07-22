//version 222222222222


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "accel_driver.h"
#include "usbcdc.h"
#include "led.h"
#include "systick.h"
#include "watchdog.h"
#include "spi0.h"

#include <string.h>

#define MAX_PWD_LEN 4

// After LIS3DH shifting, typical ranges:
// Flat: ~0–50
// Tilt 45°: ~800–1200
#define TILT_ENTER_THRESHOLD 1200
#define TILT_EXIT_THRESHOLD 800


typedef enum{
    LOCKED,
    INPUT_PWD,
    VERIFY_PWD,
    UNLOCKED
} auth_fsm_state_t;

typedef enum{
    NO_TILT,
    TILT
} tilt_fsm_state_t;

static auth_fsm_state_t auth_state = LOCKED;
static tilt_fsm_state_t tilt_state = NO_TILT;

static const char correct_pwd[] = "1234";
static char input_buffer[8];
static uint8_t input_len = 0;

uint32_t led_timer_ms = 0;
uint32_t led_period_ms = 500;

static uint32_t pwd_timer_ms = 0;
static uint32_t print_timer_ms = 0;
static bool unlocked_msg_printed = false;

void authentication_fsm(void);
void tilt_fsm(int filtered_value);
void systick_callback(void);
int16_t filter_accel_value(int16_t new_sample);




int main(void)
{
    accel_sample_t sample;

    configure_usbcdc();
    configure_led();
    configure_spi0();
    configure_accelerometer();
    configure_systick();
    configure_watchdog(500);

    register_systick_callback(systick_callback);

    while (1)
    {
        authentication_fsm();

        if (auth_state == UNLOCKED)
        {
            accelerometer_request_data();

            if (accelerometer_get_data(&sample))
            {
                // ---------------- RAW print at human speed ----------------
                /*
                if (print_timer_ms >= 200)
                {
                    printf("RAW X = %d\n\r", sample.x_axis);
                    print_timer_ms = 0;
                }
                */
                

                int16_t filtered_x = filter_accel_value(sample.x_axis);

                if (print_timer_ms >= 200)
                {
                    printf("RAW = %d   FILTERED = %d\n\r", sample.x_axis, filtered_x);
                    print_timer_ms = 0;
                }

                tilt_fsm(filtered_x);

            }
        }
    }
}



void authentication_fsm(void)
{
    char usbcdc_key = 0;

    switch (auth_state)
    {
        case LOCKED:
            input_len = 0;
            input_buffer[0] = '\0';
            pwd_timer_ms = 0;

            if (usbcdc_getchar(&usbcdc_key))
            {
                printf("Key received: %c\n", usbcdc_key);
                auth_state = INPUT_PWD;
                input_buffer[input_len++] = usbcdc_key;
            }
            break;

        case INPUT_PWD:
            while (usbcdc_getchar(&usbcdc_key))
            {
                pwd_timer_ms = 0;

                if (usbcdc_key == '\r' || usbcdc_key == '\n')
                {
                    input_buffer[input_len] = '\0';
                    auth_state = VERIFY_PWD;
                    break;
                }

                if (input_len < MAX_PWD_LEN)
                {
                    input_buffer[input_len++] = usbcdc_key;
                    printf("Key received: %c\n\r", usbcdc_key);
                }
            }
            break;

        case VERIFY_PWD:
            if (strcmp(input_buffer, correct_pwd) == 0)
            {
                auth_state = UNLOCKED;
                unlocked_msg_printed = false;
                printf("UNLOCKED!\n");
            }
            else
            {
                printf("Bad password.\n\r");
                auth_state = LOCKED;
            }
            input_len = 0;
            input_buffer[0] = '\0';
            break;

        case UNLOCKED:
            if (!unlocked_msg_printed)
            {
                printf("Authenticated Accelerometer Tilt Flash Controller is UNLOCKED\n\r");
                unlocked_msg_printed = true;
            }
            break;
    }
}



void tilt_fsm(int filtered_value)
{
    switch (tilt_state)
    {
        case NO_TILT:
            led_period_ms = 500; // slow blink
            if (filtered_value > TILT_ENTER_THRESHOLD)
                tilt_state = TILT;
            break;

        case TILT:
            led_period_ms = 75;  // fast blink
            if (filtered_value < TILT_EXIT_THRESHOLD)
                tilt_state = NO_TILT;
            break;
    }
}



void systick_callback(void)
{
    feed_the_watchdog();

    led_timer_ms++;
    print_timer_ms++;

    if (auth_state != UNLOCKED)
    {
        turn_off_led();
        led_timer_ms = 0;
        return;
    }

    if (led_timer_ms >= led_period_ms)
    {
        led_timer_ms = 0;
        toggle_led();
    }
}


int16_t filter_accel_value(int16_t new_sample)
{
    // DO NOT SHIFT HERE! Driver already shifts 12-bit left-justified values.

    // Use magnitude (FSM doesn't care about tilt direction)
    new_sample = abs(new_sample);

    // Optional noise drop (remove bottom 2 bits)
    new_sample &= ~0x03;

    static int32_t avg = 0;
    static bool initialized = false;

    if (!initialized)
    {
        avg = new_sample;
        initialized = true;
        return (int16_t)avg;
    }

    int n = 4; // smoothing factor
    avg = ((n - 1) * avg + new_sample) / n;

    return (int16_t)avg;
}


