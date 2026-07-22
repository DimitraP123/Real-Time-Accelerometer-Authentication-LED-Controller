/*
| Programmer: Dimitra Pando
| This header file defines the button driver interface, including button press types, 
| configuration, state reading, and the function to detect press durations with debouncing.
*/

#ifndef BUTTON_H 
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h> 

typedef enum
{
    NO_PRESS,
    SHORT_PRESS,
    PRESS,
    LONG_PRESS
} button_press_t;

void configure_button(void); 
_Bool button_is_down(void);
button_press_t get_button_press(void);

#endif