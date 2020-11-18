/*
 * gpio.c
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#include "../gpio.h"
#include <avr/io.h>

/// Struct representing the registers belonging to an AVR's GPIO port
typedef struct AVRGPIORegs
{
	/// Address of the PORT register for setting output value or enabling pullups
	volatile uint8_t *port;
	/// Address of the PIN register for reading input value or toggling outputs
	volatile uint8_t *pin;
	/// Address of the DDR register for setting whether pins are input or output
	volatile uint8_t *ddr;
} AVRGPIORegs;

/// The GPIO ports available on the AVR
static AVRGPIORegs const gpioRegs[] = {
	{&PORTA, &PINA, &DDRA},
	{&PORTB, &PINB, &DDRB},
	{&PORTC, &PINC, &DDRC},
	{&PORTD, &PIND, &DDRD},
	{&PORTE, &PINE, &DDRE},
	{&PORTF, &PINF, &DDRF},
};

/** Sets the direction of a GPIO pin.
 *
 * @param pin The pin
 * @param output True if it should be an output, false if it should be an input
 */
void GPIO_SetDirection(GPIOPin pin, bool output)
{
	if (output)
	{
		*(gpioRegs[pin.port].ddr) |= (1 << pin.pin);
	}
	else
	{
		*(gpioRegs[pin.port].ddr) &= ~(1 << pin.pin);
	}
}

/** Sets whether an input GPIO pin is pulled up
 *
 * @param pin The pin
 * @param pullup True if it should be pulled up, false if not
 */
void GPIO_SetPullup(GPIOPin pin, bool pullup)
{
	// On the AVR, you set pullups using the PORT register that is ordinarily
	// used for setting the output value. You just have to make sure the pin
	// is configured as an input first. Otherwise you will modify the output.
	GPIO_Set(pin, pullup);
}

/** Turns a GPIO pin on (sets it high)
 *
 * @param pin The pin
 */
void GPIO_SetOn(GPIOPin pin)
{
	*(gpioRegs[pin.port].port) |= (1 << pin.pin);
}

/** Turns a GPIO pin off (sets it low)
 *
 * @param pin The pin
 */
void GPIO_SetOff(GPIOPin pin)
{
	*(gpioRegs[pin.port].port) &= ~(1 << pin.pin);
}

/** Toggles a GPIO pin
 *
 * @param pin The pin
 */
void GPIO_Toggle(GPIOPin pin)
{
	// This is a tricky little hack the AVR provides that allows toggling
	// without a read/modify/write operation.
	*(gpioRegs[pin.port].pin) = (1 << pin.pin);
}

/** Reads the input status of a GPIO pin
 *
 * @param pin The pin
 * @return True if it's high, false if it's low
 */
bool GPIO_Read(GPIOPin pin)
{
	return *(gpioRegs[pin.port].pin) & (1 << pin.pin);
}
