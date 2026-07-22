/* 
 | Programmer: Dimitra Pando
 |
 | This program implements a synchronous (FSM) that controls
 | a servo using USBCDC. The FSM differentiates between single
 | taps and double taps of a keyboard key, specifically 'l' and
 | 'r' using systick timing.
 */

#include <stdint.h>
#include "servo.h"
#include "usbcdc.h"
#include "systick.h"

#define STEPSIZE 100
#define DOUBLE_TAP_TICK_WINDOW 200

typedef enum {
    SERVO_IDLE,
    SINGLE_KEY_TAP,
    DOUBLE_KEY_TAP
} servo_fsm_state_t;

static servo_fsm_state_t servo_state = SERVO_IDLE;
static char last_key = 0;
static uint32_t first_tap_timestamp = 0;
static int16_t current_servo_position = 0;

static void usbcdc_callback_FSM(void);

int main(void)
{
    configure_servo();
    configure_systick();
    configure_usbcdc();
    register_systick_callback(usbcdc_callback_FSM);

    while (1)
    {
        __asm__("WFI");
        if (!system_tick())
        {
            continue;
        }
    }
}

static void usbcdc_callback_FSM(void)
{
    static uint32_t tick = 0;
    uint32_t time_since_first_tap = tick - first_tap_timestamp;
    tick++;

    char usbcdc_key = 0;
    char key_pressed = 0;
    char key_received = 0;

    if (usbcdc_getchar(&usbcdc_key))
    {
        key_pressed = usbcdc_key;
        key_received = 1;
    }

    switch (servo_state)
    {
        case SERVO_IDLE:
        {
            if (key_received && (key_pressed == 'l' || key_pressed == 'r'))
            {
                last_key = key_pressed;

                first_tap_timestamp = tick;

                servo_state = SINGLE_KEY_TAP;
            }
            break;
        }

        case SINGLE_KEY_TAP:
        {
            if (time_since_first_tap > DOUBLE_TAP_TICK_WINDOW)
            {
                if (last_key == 'l')
                {
                    current_servo_position += STEPSIZE;
                }
                else if (last_key == 'r')
                {
                    current_servo_position -= STEPSIZE;
                }

                rotate_servo(current_servo_position);
                servo_state = SERVO_IDLE;
                break;
            }

            if (key_received && key_pressed == last_key)
            {
                servo_state = DOUBLE_KEY_TAP;
                break;
            }

            if (key_received && key_pressed != last_key)
            {
                if (last_key == 'l')
                {
                    current_servo_position += STEPSIZE;
                }
                else if (last_key == 'r')
                {
                    current_servo_position -= STEPSIZE;
                }

                rotate_servo(current_servo_position);
                servo_state = SERVO_IDLE;
                break;
            }

            break;
        }

        case DOUBLE_KEY_TAP:
        {
            if (last_key == 'l')
            {
                current_servo_position += (STEPSIZE / 2);
            }

            else
            {
                current_servo_position -= (STEPSIZE / 2);
            }

            rotate_servo(current_servo_position);
            servo_state = SERVO_IDLE;
            break;
        }
    }
}