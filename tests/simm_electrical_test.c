/*
 * simm_electrical_test.c
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
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

// TODO: Remember which lines shorted to ground and don't repeat those errors as being
// shorted to each other?

int SIMMElectricalTest_Run(void (*errorHandler)(uint8_t, uint8_t))
{
	// Returns number of errors found
	int numErrors = 0;

	Ports_Init();

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

	uint8_t testPinFailIndex;
	uint8_t failIndex;
	uint32_t readback = Ports_ReadAddress();
	if (readback != SIMM_ADDRESS_PINS_MASK)
	{
		failIndex = FIRST_ADDRESS_LINE_FAIL_INDEX;

		// At this point, any errors will manifest as 0 bits. It's easier to test for errors by turning them
		// into 1 bits, so invert the readback -- now, shorted pins are 1s and non-shorted pins are 0s
		readback = ~readback & SIMM_ADDRESS_PINS_MASK;

		// As long as there are any 1 bits, there is a short detected.
		while (readback)
		{
			if (readback & 1) // failure here?
			{
				errorHandler(failIndex, GROUND_FAIL_INDEX);
				numErrors++;
			}

			readback >>= 1;
			failIndex++;
		}
	}

	readback = Ports_ReadData();
	if (readback != SIMM_DATA_PINS_MASK)
	{
		failIndex = FIRST_DATA_LINE_FAIL_INDEX;

		readback = ~readback;

		while (readback)
		{
			if (readback & 1) // failure here?
			{
				errorHandler(failIndex, GROUND_FAIL_INDEX);
				numErrors++;
			}

			readback >>= 1;
			failIndex++;
		}
	}

	if (!Ports_ReadCS())
	{
		errorHandler(CS_FAIL_INDEX, GROUND_FAIL_INDEX);
		numErrors++;
	}

	if (!Ports_ReadOE())
	{
		errorHandler(OE_FAIL_INDEX, GROUND_FAIL_INDEX);
		numErrors++;
	}

	if (!Ports_ReadWE())
	{
		errorHandler(WE_FAIL_INDEX, GROUND_FAIL_INDEX);
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

		// This is the fail index of the pin we are outputting a 0 on.
		testPinFailIndex = 0;

		if (curStage == TestingAddressLines)
		{
			testPinFailIndex = FIRST_ADDRESS_LINE_FAIL_INDEX + x; // fail index of this address line
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
			testPinFailIndex = FIRST_DATA_LINE_FAIL_INDEX + x;
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
			testPinFailIndex = CS_FAIL_INDEX;
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
			testPinFailIndex = OE_FAIL_INDEX;
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
			testPinFailIndex = WE_FAIL_INDEX;
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

		DelayMS(DELAY_SETTLE_TIME_MS);

		readback = Ports_ReadAddress();
		if (curStage == TestingAddressLines)
		{
			// Insert a high bit so our test doesn't fail on the pin we were testing
			readback |= (1UL << x);
		}

		// At this point, any errors will manifest as 0 bits. It's easier to test for errors by turning them
		// into 1 bits, so invert the readback so shorted pins are 1s and non-shorted pins are 0s
		readback = ~readback & SIMM_ADDRESS_PINS_MASK;

		failIndex = FIRST_ADDRESS_LINE_FAIL_INDEX;

		// Count any shorted pins
		while (readback)
		{
			if (readback & 1) // failure here?
			{
				errorHandler(testPinFailIndex, failIndex);
				numErrors++;
			}

			readback >>= 1;
			failIndex++;
		}

		readback = Ports_ReadData();
		if (curStage == TestingDataLines)
		{
			// Insert a high bit so our test doesn't fail on the pin we were testing
			readback |= (1UL << x);
		}

		// Again, invert readback so shorted pins are 1s and non-shorted pins are 0s
		readback = ~readback;

		failIndex = FIRST_DATA_LINE_FAIL_INDEX;

		// Count any shorted pins
		while (readback)
		{
			if (readback & 1) // failure here?
			{
				errorHandler(testPinFailIndex, failIndex);
				numErrors++;
			}

			readback >>= 1;
			failIndex++;
		}

		if (curStage != TestingCS)
		{
			if (!Ports_ReadCS())
			{
				errorHandler(testPinFailIndex, CS_FAIL_INDEX);
				numErrors++;
			}
		}

		if (curStage != TestingOE)
		{
			if (!Ports_ReadOE())
			{
				errorHandler(testPinFailIndex, OE_FAIL_INDEX);
				numErrors++;
			}
		}

		if (curStage != TestingWE)
		{
			if (!Ports_ReadWE())
			{
				errorHandler(testPinFailIndex, WE_FAIL_INDEX);
				numErrors++;
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
