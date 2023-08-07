/*
 * simm_electrical_test.c
 *
 *  Created on: Nov 26, 2011
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

#include "simm_electrical_test.h"
#include "../hal/parallel_bus.h"
#include "hardware.h"
#include "board_hw.h"

/// The index of the highest SIMM address pin
#define SIMM_HIGHEST_ADDRESS_LINE			20
/// Mask that represents every SIMM address pin
#define SIMM_ADDRESS_PINS_MASK				((1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1)

/// The index of the highest SIMM data pin
#define SIMM_HIGHEST_DATA_LINE				31
/// Mask that represents every SIMM data pin
#define SIMM_DATA_PINS_MASK					0xFFFFFFFFUL

/// Milliseconds to wait before testing for shorts
#define DELAY_SETTLE_TIME_MS				20

/// The index reported as a short when it's a ground short
#define GROUND_FAIL_INDEX					0xFF
/// The index reported as a short when it's a +5V short
#define VCC_FAIL_INDEX						0xFE
/// The index reported when A0 is shorted
#define FIRST_ADDRESS_LINE_FAIL_INDEX		0
/// The index reported when A20 is shorted
#define LAST_ADDRESS_LINE_FAIL_INDEX		(FIRST_ADDRESS_LINE_FAIL_INDEX + SIMM_HIGHEST_ADDRESS_LINE)
/// The index reported when D0 is shorted
#define FIRST_DATA_LINE_FAIL_INDEX			(LAST_ADDRESS_LINE_FAIL_INDEX + 1)
/// The index reported when D31 is shorted
#define LAST_DATA_LINE_FAIL_INDEX			(FIRST_DATA_LINE_FAIL_INDEX + SIMM_HIGHEST_DATA_LINE)
/// The index reported when CS is shorted
#define CS_FAIL_INDEX						(LAST_DATA_LINE_FAIL_INDEX + 1)
/// The index reported when OE is shorted
#define OE_FAIL_INDEX						(CS_FAIL_INDEX + 1)
/// The index reported when WE is shorted
#define WE_FAIL_INDEX						(OE_FAIL_INDEX + 1)

/// Enum representing the step we're on during the electrical test
typedef enum ElectricalTestStage
{
	TestingAddressLines,//!< We are testing an address pin
	TestingDataLines,   //!< We are testing a data pin
	TestingCS,          //!< We are testing the CS pin
	TestingOE,          //!< We are testing the OE pin
	TestingWE,          //!< We are testing the WE pin
	DoneTesting         //!< We completed the test
} ElectricalTestStage;

static void SIMMElectricalTest_ResetGroundShorts(void);
static void SIMMElectricalTest_AddGroundShort(uint8_t index);
static bool SIMMElectricalTest_IsGroundShort(uint8_t index);

/// Pin indexes that were detected as shorted to ground. They have to be saved,
/// because they end up being detected as a short against every other pin,
/// so we have to be able to filter them out when testing non-ground shorts.
static uint32_t groundShorts[2];

/** Runs the electrical test
 *
 * @param errorHandler Pointer to function to call when a short is detected.
 * @return The number of errors we found
 *
 * The two parameters to errorHandler are the two indexes that are shorted.
 * (See the _FAIL_INDEX defines at the top of this file)
 */
int SIMMElectricalTest_Run(void (*errorHandler)(uint8_t, uint8_t))
{
	// Returns number of errors found
	int numErrors = 0;

	// Reset the pins we have determined that are shorted to ground
	// (We have to ignore them during the second phase of the test)
	SIMMElectricalTest_ResetGroundShorts();

	// First check for anything shorted to ground. Set all lines as inputs with
	// a weak pull-up resistor. Then read the values back and check for any
	// zeros. This would indicate a short to ground.
	ParallelBus_SetAddressDir(0);
	ParallelBus_SetDataDir(0);
	ParallelBus_SetCSDir(false);
	ParallelBus_SetOEDir(false);
	ParallelBus_SetWEDir(false);
	ParallelBus_SetAddressPullups(SIMM_ADDRESS_PINS_MASK);
	ParallelBus_SetAddressPulldowns(0);
	ParallelBus_SetDataPullups(SIMM_DATA_PINS_MASK);
	ParallelBus_SetDataPulldowns(0);
	ParallelBus_SetCSPullup(true);
	ParallelBus_SetCSPulldown(false);
	ParallelBus_SetOEPullup(true);
	ParallelBus_SetOEPulldown(false);
	ParallelBus_SetWEPullup(true);
	ParallelBus_SetWEPulldown(false);

	// Wait a brief moment...
	DelayMS(DELAY_SETTLE_TIME_MS);

	// Now loop through every pin and check it.
	uint8_t curPin = 0;
	uint8_t i;

	// Read the address pins back first
	uint32_t readback = ParallelBus_ReadAddress();
	// Check each bit for a LOW which would indicate a short to ground
	for (i = 0; i <= SIMM_HIGHEST_ADDRESS_LINE; i++)
	{
		// Did we find a low bit?
		if (!(readback & 1))
		{
			// That means this pin is shorted to ground.
			// So notify the caller that we have a ground short on this pin
			if (errorHandler)
			{
				errorHandler(curPin, GROUND_FAIL_INDEX);
			}

			// Add it to our internal list of ground shorts also.
			SIMMElectricalTest_AddGroundShort(curPin);

			// And of course increment the error counter.
			numErrors++;
		}

		// No matter what, though, move on to the next bit and pin.
		readback >>= 1;
		curPin++;
	}

	// Repeat the exact same process for the data pins
	readback = ParallelBus_ReadData();
	for (i = 0; i <= SIMM_HIGHEST_DATA_LINE; i++)
	{
		if (!(readback & 1))
		{
			if (errorHandler)
			{
				errorHandler(curPin, GROUND_FAIL_INDEX);
			}
			SIMMElectricalTest_AddGroundShort(curPin);
			numErrors++;
		}

		readback >>= 1;
		curPin++;
	}

	// Check chip select in the same way...
	if (!ParallelBus_ReadCS())
	{
		if (errorHandler)
		{
			errorHandler(curPin, GROUND_FAIL_INDEX);
		}
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++;

	// Output enable...
	if (!ParallelBus_ReadOE())
	{
		if (errorHandler)
		{
			errorHandler(curPin, GROUND_FAIL_INDEX);
		}
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++;

	// Write enable...
	if (!ParallelBus_ReadWE())
	{
		if (errorHandler)
		{
			errorHandler(curPin, GROUND_FAIL_INDEX);
		}
		SIMMElectricalTest_AddGroundShort(curPin);
		numErrors++;
	}
	curPin++; // Doesn't need to be here, but for consistency I'm leaving it.

	// OK, now we know which lines are shorted to ground. We need to keep that
	// in mind, because those lines will now show as shorted to ALL other
	// lines...ignore them during tests to find other independent shorts.

	// If the arch also supports checking for shorts to VCC, check for those next.
#if BOARD_SUPPORTS_PULLDOWNS
	ParallelBus_SetAddressPullups(0);
	ParallelBus_SetAddressPulldowns(SIMM_ADDRESS_PINS_MASK);
	ParallelBus_SetDataPullups(0);
	ParallelBus_SetDataPulldowns(SIMM_DATA_PINS_MASK);
	ParallelBus_SetCSPullup(false);
	ParallelBus_SetCSPulldown(true);
	ParallelBus_SetOEPullup(false);
	ParallelBus_SetOEPulldown(true);
	ParallelBus_SetWEPullup(false);
	ParallelBus_SetWEPulldown(true);

	// Wait a brief moment...
	DelayMS(DELAY_SETTLE_TIME_MS);

	// Now loop through every pin and check it.
	curPin = 0;

	// Read the address pins back first
	readback = ParallelBus_ReadAddress();
	// Check each bit for a HIGH which would indicate a short to VCC
	for (i = 0; i <= SIMM_HIGHEST_ADDRESS_LINE; i++)
	{
		// Did we find a high bit?
		if (readback & 1)
		{
			// That means this pin is shorted to VCC.
			// So notify the caller that we have a VCC short on this pin
			if (errorHandler)
			{
				errorHandler(curPin, VCC_FAIL_INDEX);
			}

			// And of course increment the error counter.
			numErrors++;
		}

		// No matter what, though, move on to the next bit and pin.
		readback >>= 1;
		curPin++;
	}

	// Repeat the exact same process for the data pins
	readback = ParallelBus_ReadData();
	for (i = 0; i <= SIMM_HIGHEST_DATA_LINE; i++)
	{
		if (readback & 1)
		{
			if (errorHandler)
			{
				errorHandler(curPin, VCC_FAIL_INDEX);
			}
			numErrors++;
		}

		readback >>= 1;
		curPin++;
	}

	// Check chip select in the same way...
	if (ParallelBus_ReadCS())
	{
		if (errorHandler)
		{
			errorHandler(curPin, VCC_FAIL_INDEX);
		}
		numErrors++;
	}
	curPin++;

	// Output enable...
	if (ParallelBus_ReadOE())
	{
		if (errorHandler)
		{
			errorHandler(curPin, VCC_FAIL_INDEX);
		}
		numErrors++;
	}
	curPin++;

	// Write enable...
	if (ParallelBus_ReadWE())
	{
		if (errorHandler)
		{
			errorHandler(curPin, VCC_FAIL_INDEX);
		}
		numErrors++;
	}
	curPin++; // Doesn't need to be here, but for consistency I'm leaving it.

	// Clear the pulldowns now that we're done; we won't need them anymore
	// for the rest of the testing
	ParallelBus_SetAddressPulldowns(0);
	ParallelBus_SetDataPulldowns(0);
	ParallelBus_SetCSPulldown(false);
	ParallelBus_SetOEPulldown(false);
	ParallelBus_SetWEPulldown(false);
#endif

	// Now, check each individual line vs. all other lines on the SIMM for any
	// shorts between them
	ElectricalTestStage curStage = TestingAddressLines;
	// Counter of what address or data pin we're on. Not used for control lines.
	uint8_t addrDataPin = 0;
	// What pin we are currently testing all other pins against.
	uint8_t testPin = 0;
	// More explanation: addrDataPin is only a counter inside the address or
	// data pins. testPin is a total counter of ALL pins.
	while (curStage != DoneTesting)
	{
		// Set one pin to output a 0.
		// Set all other pins as inputs with pull-ups.
		// Then read back all the other pins. If any of them read back as 0,
		// it means they are shorted to the pin we set as an output.

		// If we're testing address lines right now, set the current address
		// line as an output (and make it output a LOW). Set all other address
		// lines as inputs with pullups.
		if (curStage == TestingAddressLines)
		{
			// Mask of the address pin we're testing
			uint32_t addressLineMask = (1UL << addrDataPin);

			// Set it as an output and all other address pins as inputs.
			ParallelBus_SetAddressDir(addressLineMask);
			ParallelBus_SetAddress(0);
			ParallelBus_SetAddressPullups(~addressLineMask);
		}
		else
		{
			// If not testing an address line, set all address pins as inputs
			// with pullups. All the other stages follow the same pattern so I
			// won't bother commenting them.
			ParallelBus_SetAddressDir(0);
			ParallelBus_SetAddressPullups(SIMM_ADDRESS_PINS_MASK);
		}

		// Do the same thing for data lines...
		if (curStage == TestingDataLines)
		{
			uint32_t dataLineMask = (1UL << addrDataPin);
			ParallelBus_SetDataDir(dataLineMask);
			ParallelBus_SetData(0);
			ParallelBus_SetDataPullups(~dataLineMask);
		}
		else
		{
			ParallelBus_SetDataDir(0);
			ParallelBus_SetDataPullups(SIMM_DATA_PINS_MASK);
		}

		// Chip select...
		if (curStage == TestingCS)
		{
			ParallelBus_SetCSDir(true);
			ParallelBus_SetCS(false);
		}
		else
		{
			ParallelBus_SetCSDir(false);
			ParallelBus_SetCSPullup(true);
		}

		// Output enable...
		if (curStage == TestingOE)
		{
			ParallelBus_SetOEDir(true);
			ParallelBus_SetOE(false);
		}
		else
		{
			ParallelBus_SetOEDir(false);
			ParallelBus_SetOEPullup(true);
		}

		// And write enable.
		if (curStage == TestingWE)
		{
			ParallelBus_SetWEDir(true);
			ParallelBus_SetWE(false);
		}
		else
		{
			ParallelBus_SetWEDir(false);
			ParallelBus_SetWEPullup(true);
		}

		// OK, so now we have set up all lines as needed. Exactly one pin is
		// outputting a 0, and all other pins are inputs with pull-ups enabled.
		// Read back all the lines, and if any pin reads back as 0, it means
		// that pin is shorted to the pin we are testing (overpowering its
		// pullup). However, because we test each pin against every other pin,
		// any short would appear twice. Once as "pin 1 is shorted to pin 2"
		// and once as "pin 2 is shorted to pin 1". To avoid that annoyance, I
		// only check each combination of two pins once. You'll see below how I
		// do this by testing to ensure curPin > testPin.

		// Allow everything to settle
		DelayMS(DELAY_SETTLE_TIME_MS);

		// Now keep a count of how many pins we have actually checked during
		// THIS test iteration. This is the "fail index" of the current pin.
		curPin = 0;

		// Read back the address data to see if any shorts were found
		readback = ParallelBus_ReadAddress();

		// Count any shorted pins
		for (i = 0; i <= SIMM_HIGHEST_ADDRESS_LINE; i++)
		{
			// Failure here?
			if ((curPin > testPin) && // We haven't already checked this combination of pins (don't test pin against itself either)
				!(readback & 1) && // It's showing as low (which indicates a short)
				!SIMMElectricalTest_IsGroundShort(curPin)) // And it's not recorded as a short to ground
			{
				// Send it out as an error notification and increase error counter
				if (errorHandler)
				{
					errorHandler(testPin, curPin);
				}
				numErrors++;
			}

			// No matter what, move on to the next bit and pin
			readback >>= 1;
			curPin++;
		}

		// Same thing for data pins
		readback = ParallelBus_ReadData();

		// Count any shorted pins
		for (i = 0; i <= SIMM_HIGHEST_DATA_LINE; i++)
		{
			// Failure here?
			if ((curPin > testPin) && // We haven't already checked this combination of pins (don't test pin against itself either)
				!(readback & 1) && // It's showing as low (which indicates a short)
				!SIMMElectricalTest_IsGroundShort(curPin)) // And it's not recorded as a short to ground
			{
				if (errorHandler)
				{
					errorHandler(testPin, curPin);
				}
				numErrors++;
			}

			readback >>= 1;
			curPin++;
		}

		// And chip select...
		if ((curPin > testPin) &&
			!ParallelBus_ReadCS() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			if (errorHandler)
			{
				errorHandler(testPin, curPin);
			}
			numErrors++;
		}
		curPin++;

		// Output enable...
		if ((curPin > testPin) &&
			!ParallelBus_ReadOE() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			if (errorHandler)
			{
				errorHandler(testPin, curPin);
			}
			numErrors++;
		}
		curPin++;

		// And write enable
		if ((curPin > testPin) &&
			!ParallelBus_ReadWE() &&
			!SIMMElectricalTest_IsGroundShort(curPin))
		{
			if (errorHandler)
			{
				errorHandler(testPin, curPin);
			}
			numErrors++;
		}
		curPin++; // Not needed, kept for consistency

		// Finally, move on to the next stage if needed.
		if (curStage == TestingAddressLines)
		{
			// If we've exhausted all address lines, move on to the next stage
			// (and reset the pin counter to 0)
			if (++addrDataPin > SIMM_HIGHEST_ADDRESS_LINE)
			{
				curStage++;
				addrDataPin = 0;
			}
		}
		else if (curStage == TestingDataLines)
		{
			// If we've exhausted all data lines, move on to the next stage
			// (don't bother resetting the pin counter -- the other stages don't use it)
			if (++addrDataPin > SIMM_HIGHEST_DATA_LINE)
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

	// Restore to a normal state by calling ParallelBus_Init again.
	ParallelBus_Init();

	// Now that the final state is restored, return the number of errors found
	return numErrors;
}

/** Resets our list of pins that are shorted to ground
 *
 */
static void SIMMElectricalTest_ResetGroundShorts(void)
{
	groundShorts[0] = 0;
	groundShorts[1] = 0;
}

/** Adds a pin to the list of ground shorts
 *
 * @param index The index of the pin in the electrical test
 */
static void SIMMElectricalTest_AddGroundShort(uint8_t index)
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

/** Determines if a pin has already been saved as a ground short
 *
 * @param index The index of the pin in the electrical test
 * @return True if this pin has been determined as a short to GND, false if not
 */
static bool SIMMElectricalTest_IsGroundShort(uint8_t index)
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
