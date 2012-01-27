/*
 * main.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "external_mem.h"
#include "tests/simm_electrical_test.h"
#include "usb_serial/usb_serial.h"

int main(void)
{
	cli();

	DDRD |= (1 << 7);
	PORTD &= ~(1 << 7);

	// If there was a brownout detected, turn on the LED momentarily
	if (MCUSR & (1 << BORF))
	{
		MCUSR = 0;
		PORTD |= (1 << 7);
		_delay_ms(500);
		PORTD &= ~(1 << 7);
	}

	ExternalMem_Init();
	ExternalMem_SetAddress(0);
	ExternalMem_Assert(SIMM_CS | SIMM_OE);
	ExternalMem_Deassert(SIMM_WE);
	USBSerial_Init();
	sei();

	while (1)
	{
		USBSerial_Check();
	}

	return 0;
}
