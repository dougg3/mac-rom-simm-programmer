/*
 * gpio_hw.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#ifndef HAL_AT90USB646_GPIO_HW_H_
#define HAL_AT90USB646_GPIO_HW_H_

/// Enum representing the different GPIO ports available on the AVR.
/// Used with the GPIOPin struct.
enum {
	GPIOA,
	GPIOB,
	GPIOC,
	GPIOD,
	GPIOE,
	GPIOF
};

#endif /* HAL_AT90USB646_GPIO_HW_H_ */
