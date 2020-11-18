/*
 * led.h
 *
 *  Created on: May 27, 2012
 *      Author: Doug
 */

#ifndef LED_H_
#define LED_H_

#include "hal/board.h"
#include "hal/gpio.h"

/** Initializes the LED and turns it off
 *
 */
static inline void LED_Init(void)
{
	GPIOPin ledPin = Board_LEDPin();
	GPIO_SetDirection(ledPin, true);
	GPIO_SetOff(ledPin);
}

/** Turns the LED on
 *
 */
static inline void LED_On(void)
{
	GPIO_SetOn(Board_LEDPin());
}

/** Turns the LED off
 *
 */
static inline void LED_Off(void)
{
	GPIO_SetOff(Board_LEDPin());
}

/** Toggles the LED
 *
 */
static inline void LED_Toggle(void)
{
	GPIO_Toggle(Board_LEDPin());
}

#endif /* LED_H_ */
