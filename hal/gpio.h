/*
 * gpio.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_

#include <stdint.h>
#include <stdbool.h>

/// Creates a temporary GPIOPin struct. Used when assigning to a GPIOPin variable.
#define GPIO_PIN(port, pin)		((GPIOPin){port, pin})
/// A NULL GPIO pin
#define GPIO_PIN_NULL			((GPIOPin){0xFF, 0xFF})

/// The GPIO pin struct
typedef struct GPIOPin
{
	/// The port the pin belongs to
	uint8_t port;
	/// The index of the pin on the port
	uint8_t pin;
} GPIOPin;

void GPIO_SetDirection(GPIOPin pin, bool output);
void GPIO_SetPullup(GPIOPin pin, bool pullup);
void GPIO_SetOn(GPIOPin pin);
void GPIO_SetOff(GPIOPin pin);
void GPIO_Toggle(GPIOPin pin);
bool GPIO_Read(GPIOPin pin);

/** Sets whether a GPIO pin is outputting high or low
 *
 * @param pin The pin
 * @param on True if it's high, false if it's low
 */
static inline void GPIO_Set(GPIOPin pin, bool on)
{
	on ? GPIO_SetOn(pin) : GPIO_SetOff(pin);
}

/** Determines if a GPIO pin is null
 *
 * @param pin The pin
 * @return True if it's null, false if not
 */
static inline bool GPIO_IsNull(GPIOPin pin)
{
	return pin.pin == 0xFF && pin.port == 0xFF;
}

#endif /* HAL_GPIO_H_ */
