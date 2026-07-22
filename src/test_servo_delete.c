/* 
 | Programmer: Dimitra Pando
 | Test program to verify that the servo driver works.
 | This program does NOT use USB or keypad.
 | It simply sweeps the servo left → center → right → center forever.
 */

#include "servo.h"
#include "systick.h"
#include <stdint.h>

static void crude_delay(void)
{
    for (volatile uint32_t i = 0; i < 20000000; i++) {
        /* just burn cycles */
    }
}


void main(void)
{
    configure_systick();
    configure_servo();

    int16_t pos = 0;

    while (1)
    {


        // LEFT: -500
        rotate_servo_pin0(-1500);
        crude_delay();

        // CENTER: 0
        rotate_servo_pin0(0);
        crude_delay();

        // RIGHT: +500
        rotate_servo_pin0(1500);
        crude_delay();

        // CENTER: 0
        rotate_servo_pin0(0);
        crude_delay();
    }
}
