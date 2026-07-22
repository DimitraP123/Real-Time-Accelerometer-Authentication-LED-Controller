/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to test the keypad driver by registering the keypad scan 
 | task as a SysTick callback and outputting each pressed key in a terminal (minicom 
 | or putty).
 */

#include "keypad.h"
#include "systick.h"
#include <stdint.h>
#include "interrupt.h"
#include "usbcdc.h"
#include <stdio.h>

static void keypad_callback(void);

void main(void)
{
    configure_keypad();
    configure_systick();
    configure_usbcdc();
    register_systick_callback(keypad_callback);

    while(1)
    {
        __asm__("WFI");
        if( !system_tick() )
            continue;    
    }
}

static void keypad_callback(void)
{
    task_keypad_scan();
    static char newKeyPress = 0; 

    if (keypad_getchar(&newKeyPress))
    {
        usbcdc_putchar(newKeyPress);
        usbcdc_putchar('\n');
        usbcdc_putchar('\r');
    }
}