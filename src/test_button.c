/*
| Programmer: Dimitra Pando
| This file tests the button driver by reading button presses and controlling the LED FSM 
| to indicate short, regular, or long presses with different blink patterns or states.
*/

#include "led.h"
#include "button.h"
#include "systick.h"
#include <stdbool.h>
#include <stdint.h>

#define FAST_RATE_VALUE 100
#define SLOW_RATE_VALUE 500

void led_fsm(button_press_t press_type);

void led_fsm(button_press_t press_type)
{
    static uint16_t blink_rate = 0;
    static uint16_t led_counter = 0;

    led_counter++;

    switch(press_type)
    {
        case SHORT_PRESS:
            blink_rate = SLOW_RATE_VALUE;
            break;
        case PRESS:
            blink_rate = FAST_RATE_VALUE;
            break;
        case LONG_PRESS:
            blink_rate = 0;
            turn_off_led();
            break;
        default:
            break;
    }    

    if(led_counter >= blink_rate && blink_rate > 0)
    {
        toggle_led();
        led_counter = 0;
    }
}

int main(void)
{
    configure_button();
    configure_systick();
    configure_led();
    
    while(1)
    {
        if ( !system_tick())
            continue;

        button_press_t press_type = get_button_press();

        led_fsm(press_type);
    }
    return 0;
}