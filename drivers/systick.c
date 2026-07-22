/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to implement systick driver functions 
 | that are interrupt base instead of using the polling method.
 | This file includes registering callback and system time and system
 | tick functions. 
 */

#include <rp2350/m33.h>
#include <rp2350/ticks.h>
#include <stdbool.h>
#include <systick.h>
#include "interrupt.h"

#ifndef SYSTICK_FREQ
#define SYSTICK_FREQ_HZ 1000
#endif

#ifndef EXT_CLK_FREQ_HZ
#define EXT_CLK_FREQ_HZ 1000000
#endif

#ifndef NUM_CALLBACKS
#define NUM_CALLBACKS 5
#endif

#define SYSTICK_TOP (EXT_CLK_FREQ_HZ/SYSTICK_FREQ_HZ - 1)

#define enable_irq() __asm__("CPSIE I")
#define disable_irq() __asm__ ("CPSID I")
#define set_primask(p) __asm__ volatile ("MSR primask, %0" : : "r" (p))
#define get_primask(p) __asm__ volatile("MRS %0, primask" : "=r" (*(p)):)

static void (*callback[NUM_CALLBACKS])();
static uint32_t num_callbacks;
static volatile bool systick_flag;
static volatile uint64_t tick_counter;

void configure_systick() 
{
	TICKS_PROC0_CYCLES = 1;
	TICKS_PROC0_CTRL = TICKS_PROC0_CTRL_ENABLE(1);
	M33_SYST_RVR = SYSTICK_TOP;
	M33_SYST_CVR = 0;
	M33_SYST_CSR = 
		M33_SYST_CSR_CLKSOURCE(0) 
		| M33_SYST_CSR_TICKINT(1) 
		| M33_SYST_CSR_ENABLE(1);
}

_Bool get_system_time(void) 
{
	uint32_t primask; 
	get_primask(&primask);
	disable_irq();
	uint64_t current_tick_count = tick_counter;
	set_primask(primask);
	return current_tick_count;
}

_Bool register_systick_callback(void (*cb)())
{
	if(NUM_CALLBACKS == num_callbacks)
		return false;
	callback[num_callbacks++]=cb;
	return true;	
}

void __attribute__((isr)) SYSTICK_Handler()
{
	systick_flag=true;
	for(uint32_t i =0; i<num_callbacks; i++)
	{
		callback[i]();
	}
}

uint64_t system_tick(void)
{
	uint32_t primask;
	get_primask(&primask);
	disable_irq();
	_Bool retval = systick_flag;
	systick_flag = false;
	set_primask(primask);
	return retval;
}