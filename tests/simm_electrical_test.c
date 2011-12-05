/*
 * simm_electrical_test.c
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 */

#include "simm_electrical_test.h"
#include "../ports.h"

#define SIMM_HIGHEST_ADDRESS_LINE	18
#define SIMM_ADDRESS_PINS_MASK		((1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1)

#define SIMM_HIGHEST_DATA_LINE		31
#define SIMM_DATA_PINS_MASK			(0xFFFFFFFFUL)

typedef enum ElectricalTestStage
{
	TestingAddressLines,
	TestingDataLines,
	TestingCS,
	TestingOE,
	TestingWE,
	DoneTesting
} ElectricalTestStage;

int SIMMElectricalTest_Run(void)
{
	// Returns number of errors found
	int numErrors = 0;

	Ports_Init();

	// First check for anything shorted to ground. Set all lines as inputs with a weak pull-up resistor.
	// Then read the values back and check for any zeros. This would indicate a short to ground.
	Ports_SetAddressDDR(0);
	Ports_SetDataDDR(0);
	Ports_AddressPullups_RMW(SIMM_ADDRESS_PINS_MASK, SIMM_ADDRESS_PINS_MASK);
	Ports_DataPullups_RMW(SIMM_DATA_PINS_MASK, SIMM_DATA_PINS_MASK);

	// TODO: Wait a sec?

	if (Ports_ReadAddress() != SIMM_ADDRESS_PINS_MASK)
	{
		// TODO: Log all these errors somewhere?
		numErrors++;
	}

	if (Ports_ReadData() != SIMM_DATA_PINS_MASK)
	{
		// TODO: Log all these errors somewhere?
		numErrors++;
	}

	// Now, check each individual line vs. all other lines on the SIMM for any shorts between them
	ElectricalTestStage curStage = TestingAddressLines;
	int x = 0;
	while (curStage != DoneTesting)
	{
		// Set one pin to output a 0.
		// Set all other pins as inputs with pull-ups.
		// Then read back all the other pins. If any of them read back as 0,
		// it means they are shorted to the pin we set as an output.

		if (curStage == TestingAddressLines)
		{
			uint32_t addressLineMask = (1UL << x); // mask of the address pin we're testing

			Ports_SetAddressDDR(addressLineMask); // set it as an output and all other address pins as inputs
			Ports_AddressOut_RMW(0, addressLineMask); // set the output pin to output "0" without affecting the input pins
			Ports_AddressPullups_RMW(~addressLineMask, SIMM_ADDRESS_PINS_MASK); // turn on the pullups on all input pins
		}
		else
		{
			// If not testing an address line, set all address pins as inputs with pullups.
			// All the other stages follow the same pattern so I won't bother commenting them.
			Ports_SetAddressDDR(0);
			Ports_AddressPullups_RMW(SIMM_ADDRESS_PINS_MASK, SIMM_ADDRESS_PINS_MASK);
		}

		if (curStage == TestingDataLines)
		{
			uint32_t dataLineMask = (1UL << x);
			Ports_SetDataDDR(dataLineMask);
			Ports_DataOut_RMW(0, dataLineMask);
			Ports_DataPullups_RMW(~dataLineMask, SIMM_DATA_PINS_MASK);
		}
		else
		{
			Ports_SetDataDDR(0);
			Ports_DataPullups_RMW(SIMM_DATA_PINS_MASK, SIMM_DATA_PINS_MASK);
		}

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
		// TODO: Wait a bit here for things to settle?

		uint32_t readback = Ports_ReadAddress();
		if (curStage == TestingAddressLines)
		{
			// Insert a high bit so our test doesn't fail on the pin we were testing
			readback |= (1UL << x);
		}

		// At this point, any errors will manifest as 0 bits. It's easier to test for errors by turning them
		// into 1 bits, so invert the readback so shorted pins are 1s and non-shorted pins are 0s
		readback = ~readback & SIMM_ADDRESS_PINS_MASK;

		// Count any shorted pins
		while (readback)
		{
			numErrors++;

			// The line below turns off the rightmost bit
			// TODO: This will be useless unless I determine WHICH pin it is.
			// But this makes a good placeholder for now.
			readback = readback & (readback - 1);
		}

		readback = Ports_ReadData();
		if (curStage == TestingDataLines)
		{
			// Insert a high bit so our test doesn't fail on the pin we were testing
			readback |= (1UL << x);
		}

		// Again, invert readback so shorted pins are 1s and non-shorted pins are 0s
		readback = ~readback;

		// Count any shorted pins
		while (readback)
		{
			numErrors++;

			// The line below turns off the rightmost bit
			// TODO: This will be useless unless I determine WHICH pin it is.
			// But this makes a good placeholder for now.
			readback = readback & (readback - 1);
		}

		if (curStage != TestingCS)
		{
			if (!Ports_ReadCS())
			{
				numErrors++;

				// TODO: Report this error
			}
		}

		if (curStage != TestingOE)
		{
			if (!Ports_ReadOE())
			{
				numErrors++;

				// TODO: Report this error
			}
		}

		if (curStage != TestingWE)
		{
			if (!Ports_ReadWE())
			{
				numErrors++;

				// TODO: Report this error
			}
		}

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
	}

	return numErrors;
}
