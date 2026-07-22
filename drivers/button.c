/*
| Programmer: Dimitra Pando
| This file implements the button driver FSM that debounces an active-low 
| push-button and differentiate presses as short, regular, or long.
*/

#include "button.h"
#include <stdint.h>
#include <rp2350/resets.h>
#include <rp2350/io_bank0.h>
#include <rp2350/pads_bank0.h>
#include <rp2350/sio.h>

#ifndef DEB_PERIOD
#define DEB_PERIOD 50
#endif

#define BUTTON_PIN 1
#define BUTTON_MASK (1 << BUTTON_PIN)

#define GPIO_RESETS (RESETS_RESET_IO_BANK0_MASK | RESETS_RESET_PADS_BANK0_MASK)

#define GLITCH_PREVENTION_DEBOUNCE_THRESHOLD 50
#define SHORT_PRESS_THRESHOLD 350 

#define LONG_PRESS_THRESHOLD 1000

typedef enum
{
    CURRENTLY_NOT_BEING_PRESSED,
    CURRENTLY_BEING_PRESSED,
    PRESS_VERIFIED,
    PRESS_RELEASED
} button_state;

void configure_button(void)
{
    RESETS_RESET_CLR = GPIO_RESETS;
    while ((RESETS_RESET_DONE & GPIO_RESETS) != GPIO_RESETS)
        continue;

   pads_bank0.gpio1 = 
        PADS_BANK0_GPIO1_OD(1) |
        PADS_BANK0_GPIO1_IE(1) |
        PADS_BANK0_GPIO1_DRIVE(0) |
        PADS_BANK0_GPIO1_PUE(1) |
        PADS_BANK0_GPIO1_PDE(0) |
        PADS_BANK0_GPIO1_SCHMITT(0) |
        PADS_BANK0_GPIO1_SLEWFAST(0);
    io_bank0.gpio1_ctrl = 
        IO_BANK0_GPIO1_CTRL_IRQOVER(0) |
        IO_BANK0_GPIO1_CTRL_INOVER(0) |
        IO_BANK0_GPIO1_CTRL_OEOVER(0) |
        IO_BANK0_GPIO1_CTRL_OUTOVER(0) |
        IO_BANK0_GPIO1_CTRL_FUNCSEL(5);
}

_Bool button_is_down(void)
{
    return !(SIO_GPIO_IN & (1 << BUTTON_PIN));  
}

button_press_t get_button_press()
{ 
    static button_press_t press_type = NO_PRESS;
    static button_state button_current_state = CURRENTLY_NOT_BEING_PRESSED;
    static int press_counter = 0;
    static int release_count = 0;

    switch(button_current_state)
    {
        case CURRENTLY_NOT_BEING_PRESSED:
            if (button_is_down())
            {
                press_counter = 0;
                button_current_state = CURRENTLY_BEING_PRESSED; 
            }
            break;
        case CURRENTLY_BEING_PRESSED:
            if (button_is_down())
            {
                press_counter++;
                if (press_counter >= GLITCH_PREVENTION_DEBOUNCE_THRESHOLD)
                {
                    press_counter = 0;
                    button_current_state = PRESS_VERIFIED;
                }
            }
            else
            {
                button_current_state = CURRENTLY_NOT_BEING_PRESSED;
            }
            break;
        case PRESS_VERIFIED:
            if(button_is_down())
            {
                press_counter++;
            }
            else
            {
                release_count = 0;
                button_current_state = PRESS_RELEASED;
            }
            break;
        case PRESS_RELEASED:
            if (!button_is_down()) 
            {
                release_count++;
                if (release_count >= GLITCH_PREVENTION_DEBOUNCE_THRESHOLD)
                {
                    press_counter++;
                    if (press_counter < SHORT_PRESS_THRESHOLD) 
                    {
                        press_type = SHORT_PRESS;
                    }     
                    else if (press_counter < LONG_PRESS_THRESHOLD) 
                    {
                        press_type = PRESS;
                    }
                    else 
                    {
                        press_type = LONG_PRESS;
                    }
                    button_current_state = CURRENTLY_NOT_BEING_PRESSED;
                }
            } 
            else {
                press_type = PRESS_VERIFIED;
            }
            break;
        default:
            break;
    }
    return press_type;
}