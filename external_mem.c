/*
 * external_mem.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#include "external_mem.h"
#include "ports.h"
#include <avr/io.h>
#include <avr/delay.h>

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
	Ports_SetAddressDDR((1UL << (HIGHEST_ADDRESS_LINE + 1)) - 1);

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

void ExternalMem_Read(uint32_t startAddress, uint32_t *buf, uint32_t len)
{
	ExternalMem_AssertCS();
	ExternalMem_AssertOE();

	while (len--)
	{
		ExternalMem_SetAddress(startAddress++);
		// Shouldn't need to wait here. Each clock cycle at 16 MHz is 62.5 nanoseconds, so by the time the SPI
		// read has been signaled with the SPI chip, there will DEFINITELY be good data on the data bus.
		// (Considering these chips will be in the 70 ns or 140 ns range, that's only a few clock cycles at most)

		// TODO: Change the read data routines to put them in the correct order as is so I don't have to do this
		// (Might shave a second or so off the read time)
		uint32_t tmp = ExternalMem_ReadData();
		tmp = (tmp & 0xFF) << 24 |
			  ((tmp >> 8) & 0xFF) << 16 |
			  ((tmp >> 16) & 0xFF) << 8 |
			  ((tmp >> 24) & 0xFF) << 0;
		*buf++ = tmp;
	}
}

void ExternalMem_WriteCycle(uint32_t address, uint32_t data)
{
	ExternalMem_SetAddressAndData(address, data);
	ExternalMem_AssertWE();

	_delay_us(1); // Give it a small amount of time needed? Could I do this with some NOP instructions instead of waiting 1us?
	ExternalMem_DeassertWE();
}

void ExternalMem_UnlockAllChips(void)
{
	// Disable the chips completely and wait for a short time...
	ExternalMem_DeassertCS();
	ExternalMem_DeassertOE();
	ExternalMem_DeassertWE();
	_delay_us(1);
	ExternalMem_AssertCS();

	// First part of unlock sequence:
	// Write 0x55555555 to the address bus and 0xAA to the data bus
	// (Some datasheets may only say 0x555 or 0x5555, but they ignore
	// the upper bits, so writing the alternating pattern to all address lines
	// should make it compatible with larger chips)
	ExternalMem_WriteCycle(0x55555555UL, 0xAAAAAAAAUL);

	// Second part of unlock sequence is the same thing, but reversed.
	ExternalMem_WriteCycle(0xAAAAAAAAUL, 0x55555555UL);
}

void ExternalMem_IdentifyChips(struct ChipID *chips)
{
	ExternalMem_UnlockAllChips();

	// Write 0x90 to 0x55555555 for the identify command...
	ExternalMem_WriteCycle(0x55555555UL, 0x90909090UL);

	// Now we can read the vendor and product ID
	ExternalMem_SetAddress(0);
	ExternalMem_SetDataAsInput();
	ExternalMem_AssertOE();

	uint32_t result = ExternalMem_ReadData();

	chips[3].manufacturerID = (uint8_t)result;
	chips[2].manufacturerID = (uint8_t)(result >> 8);
	chips[1].manufacturerID = (uint8_t)(result >> 16);
	chips[0].manufacturerID = (uint8_t)(result >> 24);

	ExternalMem_SetAddress(1);
	result = ExternalMem_ReadData();

	chips[3].deviceID = (uint8_t)result;
	chips[2].deviceID = (uint8_t)(result >> 8);
	chips[1].deviceID = (uint8_t)(result >> 16);
	chips[0].deviceID = (uint8_t)(result >> 24);

	// Exit software ID mode
	ExternalMem_DeassertOE();
	ExternalMem_WriteCycle(0, 0xF0F0F0F0UL);
}
