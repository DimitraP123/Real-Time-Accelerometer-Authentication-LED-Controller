/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to declare systick driver functions,
 | needed to configure and use the systick timer that is interrupt
 | based. Additionally, this file defines macros and constants.
 */

#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdbool.h>
#include <stdint.h>
#include "interrupt.h"

void configure_systick(void);
_Bool register_systick_callback( void (*cb)(void) );
_Bool get_system_time(void);
uint64_t system_tick(void); 
void __attribute__((isr)) SYSTICK_Handler();

#endif