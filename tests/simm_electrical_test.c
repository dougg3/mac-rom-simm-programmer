/*
 * simm_electrical_test.c
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 */

#include "simm_electrical_test.h"
#include "../ports.h"

#define SIMM_HIGHEST_ADDRESS_LINE	18

int SIMMElectricalTest_Run(void)
{
	// Returns number of errors found
	int numErrors = 0;

	Ports_Init();

	// First check for anything shorted to ground. Set all lines as inputs with a weak pull-up resistor.
	// Then read the values back and check for any zeros. This would indicate a short to ground.
	Ports_SetAddressDDR(0);
	Ports_SetDataDDR(0);
	Ports_AddressPullups_RMW((1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1, (1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1);
	Ports_DataPullups_RMW(0xFFFFFFFFUL, 0xFFFFFFFFUL);

	// TODO: Wait a sec?

	if (Ports_ReadAddress() != (1UL << (SIMM_HIGHEST_ADDRESS_LINE + 1)) - 1)
	{
		// TODO: Log all these errors somewhere?
		numErrors++;
	}

	if (Ports_ReadData() != 0xFFFFFFFFUL)
	{
		// TODO: Log all these errors somewhere?
		numErrors++;
	}

	return numErrors;
}
