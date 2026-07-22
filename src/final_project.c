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
#define LED_ACCEL_THRESHOLD 1000


typedef enum{
    LOCKED,
    INPUT_PWD,
    VERIFY_PWD,
    UNLOCKED
} auth_fsm_state_t;

typedef enum {
    LOW_ACCEL_VAL,
    HIGH_ACCEL_VAL
} led_fsm_state_t;

static auth_fsm_state_t auth_state = LOCKED;
static led_fsm_state_t led_state = LOW_ACCEL_VAL;

static const char correct_pwd[] = "1234";
static char input_buffer[8];
static uint8_t input_len = 0;

uint32_t led_timer_ms = 0;
uint32_t led_period_ms = 500;
static uint32_t print_timer_ms = 0;

static bool unlocked_msg_printed = false;

void authentication_fsm(void);
void led_speed_fsm(int accel_mag);
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
            accel_data_request();

            if (accel_data_retrieve(&sample))
            {
                int16_t filtered_x = filter_accel_value(sample.x_axis);

                if (print_timer_ms >= 200)
                {
                    printf("FILTERED = %d\n\r", filtered_x);

                    led_speed_fsm(filtered_x);

                    print_timer_ms = 0;
                }
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
                if (usbcdc_key == '\r' || usbcdc_key == '\n')
                {
                    input_buffer[input_len] = '\0';
                    auth_state = VERIFY_PWD;
                    break;
                }

                if (input_len < MAX_PWD_LEN)
                {
                    input_buffer[input_len++] = usbcdc_key;
                    printf("Key received: %c\r\n", usbcdc_key);
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
                printf("Bad password.\r\n");
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


void led_speed_fsm(int accel_mag)
{
    switch (led_state)
    {
        case LOW_ACCEL_VAL:
        {
            led_period_ms = 250;

            if (accel_mag > LED_ACCEL_THRESHOLD)
            {
                led_state = HIGH_ACCEL_VAL;
            }
                
            break;
        }
            

        case HIGH_ACCEL_VAL:
        {
            led_period_ms = 75;
                
            if (accel_mag < LED_ACCEL_THRESHOLD)
            {
                led_state = LOW_ACCEL_VAL;
            }
                
            break;
        }
          
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
    
    new_sample = abs(new_sample);
    new_sample &= ~0x03;

    static int32_t avg = 0;
    static bool initialized = false;

    if (!initialized)
    {
        avg = new_sample;
        initialized = true;
        return (int16_t)avg;
    }

    int n = 4; 
    avg = ((n - 1) * avg + new_sample) / n;

    return (int16_t)avg;
}


