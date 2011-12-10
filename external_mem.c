/*
 * external_mem.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#include "external_mem.h"
#include "ports.h"
#include <avr/io.h>

#define HIGHEST_ADDRESS_LINE	20

// Allow this to be initialized more than once.
// In case we mess with the port settings,
// re-initializing ExternalMem should reset everything
// to sensible defaults.
void ExternalMem_Init(void)
{
	// Initialize the ports connected to address/data/control lines
	Ports_Init();

	// Configure all address lines as outputs
	Ports_SetAddressDDR((1UL << (HIGHEST_ADDRESS_LINE - 1)) - 1);

	// Set all data lines as inputs
	Ports_SetDataDDR(0);

	// Disable all pull-ups on the data lines. They aren't needed
	// for normal operation.
	Ports_DataPullups_RMW(0, 0xFFFFFFFFUL);

	// Sensible defaults for address lines:
	// Write out address zero
	Ports_SetAddressOut(0);

	// Control lines
	Ports_SetCSDDR(1);
	Ports_SetOEDDR(1);
	Ports_SetWEDDR(1);

	// Default all control lines to high (de-asserted)
	ExternalMem_DeassertCS();
	ExternalMem_DeassertOE();
	ExternalMem_DeassertWE();
}

void ExternalMem_SetAddress(uint32_t address)
{
	Ports_SetAddressOut(address);
}

void ExternalMem_SetData(uint32_t data)
{
	Ports_SetDataDDR(0xFFFFFFFFUL);
	Ports_SetDataOut(data);
}

void ExternalMem_SetAddressAndData(uint32_t address, uint32_t data)
{
	ExternalMem_SetAddress(address);
	ExternalMem_SetData(data);
}

void ExternalMem_SetDataAsInput(void)
{
	Ports_SetDataDDR(0);
}

uint32_t ExternalMem_ReadData(void)
{
	return Ports_ReadData();
}

void ExternalMem_AssertCS(void)
{
	Ports_SetCSOut(0);
}

void ExternalMem_DeassertCS(void)
{
	Ports_SetCSOut(1);
}

void ExternalMem_AssertWE(void)
{
	Ports_SetWEOut(0);
}

void ExternalMem_DeassertWE(void)
{
	Ports_SetWEOut(1);
}

void ExternalMem_AssertOE(void)
{
	Ports_SetOEOut(0);
}

void ExternalMem_DeassertOE(void)
{
	Ports_SetOEOut(1);
}
