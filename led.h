/*
 * led.h
 *
 *  Created on: May 27, 2012
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
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
