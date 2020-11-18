/*
 * hardware.h
 *
 *  Created on: Dec 4, 2011
 *      Author: Doug
 */

#ifndef HAL_AT90USB646_HARDWARE_H_
#define HAL_AT90USB646_HARDWARE_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/** Disables interrupts
 *
 */
static inline void DisableInterrupts(void)
{
	cli();
}

/** Enables interrupts
 *
 */
static inline void EnableInterrupts(void)
{
	sei();
}

/** Blocks for the specified number of milliseconds
 *
 * @param ms The number of milliseconds to wait
 */
static inline void DelayMS(uint16_t ms)
{
	_delay_ms(ms);
}

/** Blocks for the specified number of microseconds
 *
 * @param us The number of microseconds to wait
 */
static inline void DelayUS(uint16_t us)
{
	_delay_us(us);
}

#endif /* HAL_AT90USB646_HARDWARE_H_ */
