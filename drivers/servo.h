/* 
 | Programmer: Dimitra Pando
 |
 | This purpose of this file is to declare function 
 | prototypes used in the servo driver. Including 
 | the prototypes used to configure the hardware and 
 | positioning the servo.
 */

#ifndef SERVO_H 
#define SERVO_H

#include <stdbool.h>
#include <stdint.h> 

void configure_servo(void);
void rotate_servo(int16_t rotate);

#endif
