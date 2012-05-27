/*
 * main.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2012 Doug Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "external_mem.h"
#include "tests/simm_electrical_test.h"
#include "usb_serial/usb_serial.h"
#include "led.h"

int main(void)
{
	cli();

	LED_Init();

	// If there was a brownout detected, turn on the LED momentarily
	if (MCUSR & (1 << BORF))
	{
		MCUSR = 0;
		LED_On();
		_delay_ms(500);
		LED_Off();
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
