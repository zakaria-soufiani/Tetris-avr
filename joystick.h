/*
 * joystick.h
 *
 * Created: 6/1/2016 10:02:39 PM
 *  Author: manlylaptops.com
 */ 


#ifndef JOYSTICK_H_
#define JOYSTICK_H_



#include <stdint.h>

void init_joystick(void);
void measure_joystick(void);
int8_t joystick_direction(void);

#endif /* JOYSTICK_H_ */
