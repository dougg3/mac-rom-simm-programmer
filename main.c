/*
 * main.c
 *
 *  Created on: Nov 25, 2011
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
 * -----------------------------------------------------------------------------
 *
 * TODO: Add smarter short detection? Automatically run an electrical test at
 *       startup and leave everything in input mode if shorts are detected?
 *       I'm especially thinking about the case of SIMM control pins shorted
 *       together, like CS and OE, which will default to opposite output values.
 *       Is this even worth implementing? It's probably only useful when testing
 *       newly-built SIMMs. We would need to implement a protocol for this so
 *       the programmer software can be alerted that a short was detected.
 */

#include "hal/board.h"
#include "hardware.h"
#include "hal/parallel_bus.h"
#include "tests/simm_electrical_test.h"
#include "simm_programmer.h"
#include "led.h"

/** Main function
 *
 * @return Never; the main loop is an infinite loop.
 */
int main(void)
{
	DisableInterrupts();
	Board_Init();
	LED_Init();

	// If there was a brownout detected, turn on the LED momentarily
	if (Board_BrownoutDetected())
	{
		LED_On();
		DelayMS(500);
		LED_Off();
	}

	// Initialize everything and turn on interrupts
	ParallelBus_Init();
	SIMMProgrammer_Init();
	EnableInterrupts();

	// Main loop
	while (1)
	{
		SIMMProgrammer_Check();
	}

	return 0;
}
