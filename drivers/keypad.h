/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to declare the prototype used in the keypad driver, 
 | such as the prototypes needed to configure the keypad hardware, scan the rows  
 | and columns of the keypad, and retrieve the pressed key values
 */


#ifndef KEYPAD_H 
#define KEYPAD_H

#include <stdbool.h>
#include <stdint.h> 

void configure_keypad(void);
void task_keypad_scan(void);
int keypad_getchar( char *c );

#endif
