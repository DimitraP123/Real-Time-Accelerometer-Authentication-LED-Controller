/*
| Programmer: Dimitra Pando
|
| This file tests the functionality of the systick driver
| and tests that the systick interrupts are working by
| using callbacks and the function get_system_time(). 
| There are two callbacks, one that uses the LEDs and 
| one that uses a print statement and USBCDC.
*/

#include "systick.h"
#include "led.h"
#include <stdint.h>
#include "interrupt.h"
#include "usbcdc.h"
#include <stdio.h>

void main(void)
{
    __disable_irq();
    configure_led();
    configure_systick();
    configure_usbcdc();
    register_systick_callback(led_callback);
    register_systick_callback(print_callback);
    __enable_irq();

    while(1)
    {
        __asm__("WFI");
        if( !system_tick() )
        continue;        
    }
}

static void led_callback(void)
{
    static uint16_t led_count = 0;
    led_count++;
    if (led_count >= 500) {       
        toggle_led(0);
        led_count = 0;
    }
}

static void print_callback(void)
{
    static uint16_t print_count = 0;
    print_count++;
    if (print_count >= 1000) {     
        uint64_t system_time = get_system_time();
        printf("2nd SysTick callback is active\n");
        print_count = 0;
    }
} 