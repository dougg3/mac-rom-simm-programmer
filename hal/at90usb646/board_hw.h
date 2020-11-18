/*
 * board_hw.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#ifndef HAL_AT90USB646_BOARD_HW_H_
#define HAL_AT90USB646_BOARD_HW_H_

#include "gpio_hw.h"
#include "../gpio.h"

/** Gets the GPIO pin on the board that controls the status LED
 *
 * @return The status LED pin
 */
static inline GPIOPin Board_LEDPin(void)
{
	return GPIO_PIN(GPIOD, 7);
}

#endif /* HAL_AT90USB646_BOARD_HW_H_ */
