/*
 * simm_electrical_test.c
 *
 *  Created on: Nov 26, 2011
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

#include "simm_electrical_test.h"
#include "../ports.h"
#include "../delay.h"

#define SIMM_HIGHEST_ADDRESS_LINE	20
#define SIMM_ADDRESS_PINS_MASK		((1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1)

#define SIMM_HIGHEST_DATA_LINE		31
#define SIMM_DATA_PINS_MASK			(0xFFFFFFFFUL)

#define DELAY_SETTLE_TIME_MS		20

typedef enum ElectricalTestStage
{
	TestingAddressLines,
	TestingDataLines,
	TestingCS,
	TestingOE,
	TestingWE,
	DoneTesting
} ElectricalTestStage;

// Private functions
void SIMMElectricalTest_ResetGroundShorts(void);
void SIMMElectricalTest_AddGroundShort(uint8_t index);
bool SIMMElectricalTest_IsGroundShort(uint8_t index);

int SIMMElectricalTest_Run(void (*errorHandler)(uint8_t, uint8_t))
{
	// Returns number of errors found
	int numErrors = 0;

	// Pins we have determined are shorted to ground
	// (We have to ignore them during the second phase of the test)
	SIMMElectricalTest_ResetGroundShorts();

	Ports_Init();

	// Give everything a bit of time to settle down
	DelayMS(DELAY_SETTLE_TIME_MS);

	// First check for anything shorted to ground. Set all lines as inputs with a weak pull-up resistor.
	// Then read the values back and check for any zeros. This would indicate a short to ground.
	Ports_SetAddressDDR(0);
	Ports_SetDataDDR(0);
	Ports_SetCSDDR(false);
	Ports_SetOEDDR(false);
	Ports_SetWEDDR(false);
	Ports_AddressPullups_RMW(SIMM_ADDRESS_PINS_MASK, SIMM_ADDRESS_PINS_MASK);
	Ports_DataPullups_RMW(SIMM_DATA_PINS_MASK, SIMM_DATA_PINS_MASK);
	Ports_SetCSPullup(true);
	Ports_SetOEPullup(true);
	Ports_SetWEPullup(true);

	DelayMS(DELAY_SETTLE_TIME_MS);

	uint8_t curPin = 0;
	uint8_t i;

	// Read the address pins back first of all
	uint32_t readback = Ports_ReadAddress();
	if (readback != SIMM_ADDRESS_PINS_MASK)
	{
		// Check each bit for a LOW which would indicate a short to ground
		for (i = 0; i <= SIMM_HIGHEST_ADDRESS_LINE; i++)
		{
			// Did we find a low bit?
			if (!(readback & 1))
			{
				// That means this pin is shorted to ground.
				// So notify the caller that we have a ground short on this pin
				errorHandler(curPin, GROUND_FAIL_INDEX);

				// Add it to our internal list of ground shorts also.
				SIMMElectricalTest_AddGroundShort(curPin);

				// And of course increment the error counter.
				numErrors++;
			}

			// No matter what, though, move on to the next bit and pin.
			readback >>= 1;
			curPin++;
		}
	}

	// Repeat the exact same process for the data pins
	readback = Ports_ReadData();
	if (readback != SIMM_DATA_PINS_MASK)
	{
		for (i = 0; i <= SIMM_HIGHEST_DATA_LINE; i++)
		{
			if (!(readback & 1)) // failure here?
			{
				errorHandler(curPin, GROUND_FAIL_INDEX);
				SIMMElectricalTest_AddGroundShort(curPin);
				numErrors++;
			}

			readback >>= 1;
			curPin++;
		}
	}

	// Check chip select in the same way...
	if (!Ports_ReadCS())
	{
		errorHandler(curPin, GROUND_FAIL_INDEX);
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++;

	// Output enable...
	if (!Ports_ReadOE())
	{
		errorHandler(curPin, GROUND_FAIL_INDEX);
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++;

	// Write enable...
	if (!Ports_ReadWE())
	{
		errorHandler(curPin, GROUND_FAIL_INDEX);
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++; // Doesn't need to be here, but for consistency I'm leaving it.

	// OK, now we know which lines are shorted to ground.
	// We need to keep that in mind, because those lines will now show as shorted
	// to ALL other lines...ignore them during tests to find other independent shorts

	// Now, check each individual line vs. all other lines on the SIMM for any shorts between them
	ElectricalTestStage curStage = TestingAddressLines;
	int x = 0; // Counter of what address or data pin we're on. Not used for control lines.
	uint8_t testPin = 0; // What pin we are currently testing all other pins against.
	// x is only a counter inside the address or data pins.
	// testPin is a total counter of ALL pins.
	while (curStage != DoneTesting)
	{
		// Set one pin to output a 0.
		// Set all other pins as inputs with pull-ups.
		// Then read back all the other pins. If any of them read back as 0,
		// it means they are shorted to the pin we set as an output.

		// If we're testing address lines right now, set the current address line
		// as an output (and make it output a LOW).
		// Set all other address lines as inputs with pullups.
		if (curStage == TestingAddressLines)
		{
			uint32_t addressLineMask = (1UL << x); // mask of the address pin we're testing

			Ports_SetAddressDDR(addressLineMask); // set it as an output and all other address pins as inputs
			Ports_AddressOut_RMW(0, addressLineMask); // set the output pin to output "0" without affecting the input pins
			Ports_AddressPullups_RMW(SIMM_ADDRESS_PINS_MASK, ~addressLineMask); // turn on the pullups on all input pins
		}
		else
		{
			// If not testing an address line, set all address pins as inputs with pullups.
			// All the other stages follow the same pattern so I won't bother commenting them.
			Ports_SetAddressDDR(0);
			Ports_AddressPullups_RMW(SIMM_ADDRESS_PINS_MASK, SIMM_ADDRESS_PINS_MASK);
		}

		// Do the same thing for data lines...
		if (curStage == TestingDataLines)
		{
			uint32_t dataLineMask = (1UL << x);
			Ports_SetDataDDR(dataLineMask);
			Ports_DataOut_RMW(0, dataLineMask);
			Ports_DataPullups_RMW(SIMM_DATA_PINS_MASK, ~dataLineMask);
		}
		else
		{
			Ports_SetDataDDR(0);
			Ports_DataPullups_RMW(SIMM_DATA_PINS_MASK, SIMM_DATA_PINS_MASK);
		}

		// Chip select...
		if (curStage == TestingCS)
		{
			Ports_SetCSDDR(true);
			Ports_SetCSOut(false);
		}
		else
		{
			Ports_SetCSDDR(false);
			Ports_SetCSPullup(true);
		}

		// Output enable...
		if (curStage == TestingOE)
		{
			Ports_SetOEDDR(true);
			Ports_SetOEOut(false);
		}
		else
		{
			Ports_SetOEDDR(false);
			Ports_SetOEPullup(true);
		}

		// And write enable.
		if (curStage == TestingWE)
		{
			Ports_SetWEDDR(true);
			Ports_SetWEOut(false);
		}
		else
		{
			Ports_SetWEDDR(false);
			Ports_SetWEPullup(true);
		}

		// OK, so now we have set up all lines as needed. Exactly one pin is outputting a 0, and all other pins
		// are inputs with pull-ups enabled. Read back all the lines, and if any pin reads back as 0,
		// it means that pin is shorted to the pin we are testing (overpowering its pullup)
		// However, because we test each pin against every other pin, any short would appear twice.
		// Once as "pin 1 is shorted to pin 2" and once as "pin 2 is shorted to pin 1". To avoid
		// that annoyance, I only check each combination of two pins once. You'll see below
		// how I do this by testing to ensure curPin > testPin.

		DelayMS(DELAY_SETTLE_TIME_MS);

		// Now keep a count of how many pins we have actually checked during THIS test.
		curPin = 0;

		// Read back the address data to see if any shorts were found
		readback = Ports_ReadAddress();

		// Count any shorted pins
		for (i = 0; i <= SIMM_HIGHEST_ADDRESS_LINE; i++)
		{
			// failure here?
			if ((curPin > testPin) && // We haven't already checked this combination of pins (don't test pin against itself either)
				!(readback & 1) && // It's showing as low (which indicates a short)
				!SIMMElectricalTest_IsGroundShort(curPin)) // And it's not recorded as a short to ground
			{
				// Send it out as an error notification and increase error counter
				errorHandler(testPin, curPin);
				numErrors++;
			}

			// No matter what, move on to the next bit and pin
			readback >>= 1;
			curPin++;
		}

		// Same thing for data pins
		readback = Ports_ReadData();

		// Count any shorted pins
		for (i = 0; i <= SIMM_HIGHEST_DATA_LINE; i++)
		{
			// failure here?
			if ((curPin > testPin) && // We haven't already checked this combination of pins (don't test pin against itself either)
				!(readback & 1) && // It's showing as low (which indicates a short)
				!SIMMElectricalTest_IsGroundShort(curPin)) // And it's not recorded as a short to ground
			{
				errorHandler(testPin, curPin);
				numErrors++;
			}

			readback >>= 1;
			curPin++;
		}

		// And chip select...
		if ((curPin > testPin) &&
			!Ports_ReadCS() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			errorHandler(testPin, curPin);
			numErrors++;
		}
		curPin++;

		// Output enable...
		if ((curPin > testPin) &&
			!Ports_ReadOE() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			errorHandler(testPin, curPin);
			numErrors++;
		}
		curPin++;

		// And write enable
		if ((curPin > testPin) &&
			!Ports_ReadWE() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			errorHandler(testPin, curPin);
			numErrors++;
		}
		curPin++; // Not needed, kept for consistency

		// Finally, move on to the next stage if needed.
		if (curStage == TestingAddressLines)
		{
			// If we've exhausted all address lines, move on to the next stage
			// (and reset the pin counter to 0)
			if (++x > SIMM_HIGHEST_ADDRESS_LINE)
			{
				curStage++;
				x = 0;
			}
		}
		else if (curStage == TestingDataLines)
		{
			// If we've exhausted all data lines, move on to the next stage
			// (don't bother resetting the pin counter -- the other stages don't use it)
			if (++x > SIMM_HIGHEST_DATA_LINE)
			{
				curStage++;
			}
		}
		else
		{
			curStage++;
		}

		// Move on to test the next pin
		testPin++;
	}

	// Restore to a normal state. Disable any pull-ups, return CS/OE/WE to outputs,
	// deassert them all, set data as an input, set address as an input,
	// assert CS and OE. Should be a pretty normal state.

	// To start, make everything an input so we can disable pullups.
	Ports_SetCSDDR(false);
	Ports_SetOEDDR(false);
	Ports_SetWEDDR(false);
	Ports_SetAddressDDR(0);
	Ports_SetDataDDR(0);

	// Disable pullups

	Ports_SetCSPullup(false);
	Ports_SetOEPullup(false);
	Ports_SetWEPullup(false);
	Ports_AddressPullups_RMW(0, SIMM_ADDRESS_PINS_MASK);
	Ports_DataPullups_RMW(0, SIMM_DATA_PINS_MASK);

	// Set control lines to deasserted outputs (remember ON is deasserted)
	Ports_SetCSDDR(true);
	Ports_SetOEDDR(true);
	Ports_SetWEDDR(true);
	Ports_SetCSOut(true);
	Ports_SetOEOut(true);
	Ports_SetWEOut(true);

	// Set address lines to outputs and set the outputted address to zero
	Ports_SetAddressDDR(SIMM_ADDRESS_PINS_MASK);
	Ports_SetAddressOut(0);

	// Leave data lines as inputs...(they're already inputs so do nothing...)

	// Now assert CS and OE so we're in normal "read" mode
	Ports_SetCSOut(false);
	Ports_SetOEOut(false);

	// Now that the final state is restored, return the number of errors found
	return numErrors;
}

// Stuff for handling shorts to ground
// (They have to be remembered because the ground shorts will be repeated
// when you test each individual pin against all other pins)
static uint32_t groundShorts[2];

void SIMMElectricalTest_ResetGroundShorts(void)
{
	groundShorts[0] = 0;
	groundShorts[1] = 0;
}

void SIMMElectricalTest_AddGroundShort(uint8_t index)
{
	if (index < 32)
	{
		groundShorts[0] |= (1UL << index);
	}
	else if (index < 64)
	{
		groundShorts[1] |= (1UL << (index - 32));
	}
	// None are >= 64, no further handling needed
}

bool SIMMElectricalTest_IsGroundShort(uint8_t index)
{
	if (index < 32)
	{
		return ((groundShorts[0] & (1UL << index)) != 0);
	}
	else if (index < 64)
	{
		return ((groundShorts[1] & (1UL << (index - 32))) != 0);
	}
	else
	{
		return false;
	}
}
