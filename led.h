/*
 * led.h
 *
 *  Created on: May 27, 2012
 *      Author: Doug
 */

#ifndef LED_H_
#define LED_H_

#include <avr/io.h>
#define PIN_MASK				(1 << 7)

#define LED_Init()				do { DDRD |= PIN_MASK; LED_Off(); } while (0)
#define LED_On()				PORTD |= PIN_MASK
#define LED_Off()				PORTD &= ~PIN_MASK
#define LED_Toggle()			PIND = PIN_MASK

#endif /* LED_H_ */
