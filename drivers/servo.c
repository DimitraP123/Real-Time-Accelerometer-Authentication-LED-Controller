/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to implement the low-level driver 
 | for a servo motor. It configures GPIO, pads, and the PWM
 | hardware necessary. This file also contains the rotate_servo() 
 | function, which is used to position the servo motor.
 */

#include "servo.h"
#include <rp2350/resets.h>
#include <rp2350/sio.h>
#include <rp2350/io_bank0.h>
#include <rp2350/pads_bank0.h>
#include <rp2350/pwm.h>
#include <stdint.h>

#define SERVO_OFFSET 290
#define SERVO_MAX (2500 + SERVO_OFFSET)
#define SERVO_ZERO (1500 + SERVO_OFFSET)
#define SERVO_MIN (500 + SERVO_OFFSET)

#define SERVO_RESETS (RESETS_RESET_IO_BANK0_MASK | RESETS_RESET_PADS_BANK0_MASK | RESETS_RESET_PWM_MASK)

void configure_servo(void)
{
    RESETS_RESET_CLR = SERVO_RESETS;

    while((RESETS_RESET_DONE & SERVO_RESETS ) != SERVO_RESETS )
    	continue;

    PADS_BANK0_GPIO0 = 
	PADS_BANK0_GPIO0_OD(0) 
	| PADS_BANK0_GPIO0_IE(0) 
	| PADS_BANK0_GPIO0_DRIVE(0) 
	| PADS_BANK0_GPIO0_PUE(0) 
	| PADS_BANK0_GPIO0_PDE(0) 
	| PADS_BANK0_GPIO0_SCHMITT(0) 
	| PADS_BANK0_GPIO0_SLEWFAST(0);

    IO_BANK0_GPIO0_CTRL = 
	IO_BANK0_GPIO0_CTRL_IRQOVER(0) |
	IO_BANK0_GPIO0_CTRL_INOVER(0)  |
	IO_BANK0_GPIO0_CTRL_OEOVER(0)  |
	IO_BANK0_GPIO0_CTRL_OUTOVER(0) |
	IO_BANK0_GPIO0_CTRL_FUNCSEL(4); 

    PWM_CH0_DIV = PWM_CH0_DIV_INT(144) | PWM_CH0_DIV_FRAC(0);
    PWM_CH0_TOP = 49999;

    rotate_servo(0);   

    PWM_CH0_CSR =
        PWM_CH0_CSR_PH_ADV(0)
        | PWM_CH0_CSR_PH_RET(0)
        | PWM_CH0_CSR_DIVMODE(0)
        | PWM_CH0_CSR_B_INV(0)   
        | PWM_CH0_CSR_A_INV(0)
        | PWM_CH0_CSR_PH_CORRECT(0)
        | PWM_CH0_CSR_EN(1);
}

void rotate_servo(int16_t rotate)
{
    int32_t duty = SERVO_ZERO + rotate;

    if(duty > SERVO_MAX)
    {
        duty = SERVO_MAX;
    }

    else if(duty < SERVO_MIN)
    {
        duty = SERVO_MIN;
    }
        
    PWM_CH0_CC = PWM_CH0_CC_A(duty);
}