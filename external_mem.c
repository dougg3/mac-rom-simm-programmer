/*
 * external_mem.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#include "external_mem.h"
#include "ports.h"
#include <avr/io.h>
#include <util/delay.h>

#define HIGHEST_ADDRESS_LINE	20

// Private functions
uint32_t ExternalMem_MaskForChips(uint8_t chips);
void ExternalMem_WaitCompletion(uint8_t chipsMask);

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
	/*ExternalMem_DeassertCS();
	ExternalMem_DeassertOE();
	ExternalMem_DeassertWE();*/
	ExternalMem_Deassert(SIMM_CS | SIMM_OE | SIMM_WE);
}

void ExternalMem_SetAddress(uint32_t address)
{
	Ports_SetAddressOut(address);
}

void ExternalMem_SetData(uint32_t data)
{
	Ports_SetDataDDR(0xFFFFFFFFUL);
	Ports_SetDataOut(data);
	/*DDRE = 0xFF;
	DDRF = 0xFF;
	PORTE = (data & 0xFF);
	PORTF = ((data >> 8) & 0xFF);*/
}

void ExternalMem_SetAddressAndData(uint32_t address, uint32_t data)
{
	ExternalMem_SetAddress(address);
	ExternalMem_SetData(data);
}

void ExternalMem_SetDataAsInput(void)
{
	Ports_SetDataDDR(0);
	//DDRE = 0;
	//DDRF = 0;
}

uint32_t ExternalMem_ReadData(void)
{
	return Ports_ReadData();
	//return (uint32_t)PINE | (((uint32_t)PINF) << 8);
}

/*void ExternalMem_AssertCS(void)
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
}*/

void ExternalMem_Read(uint32_t startAddress, uint32_t *buf, uint32_t len)
{
	// This is just a time saver if we know we will
	// be reading a complete block -- doesn't bother
	// playing with the control lines between each byte
	//ExternalMem_DeassertWE();
	//ExternalMem_AssertCS();
	//ExternalMem_AssertOE();
	ExternalMem_Deassert(SIMM_WE);
	ExternalMem_Assert(SIMM_CS | SIMM_OE);
	ExternalMem_SetDataAsInput();

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
	/*ExternalMem_AssertCS();
	ExternalMem_DeassertOE();*/
	ExternalMem_Assert(SIMM_CS);
	ExternalMem_Deassert(SIMM_OE | SIMM_WE);
	ExternalMem_SetAddressAndData(address, data);
	ExternalMem_Assert(SIMM_WE);
	//ExternalMem_AssertWE();
	//_delay_us(1); // Give it a small amount of time needed? Could I do this with some NOP instructions instead of waiting 1us?
// The minimum pulse time is 50 ns or so, and since one clock cycle is
	// 62.5 ns, this should be fine to assert and deassert immediately
	//ExternalMem_DeassertWE();
	ExternalMem_Deassert(SIMM_WE);
}

uint32_t ExternalMem_ReadCycle(uint32_t address)
{
	//ExternalMem_DeassertWE();
	//ExternalMem_AssertCS();
	//ExternalMem_AssertOE();
	ExternalMem_Deassert(SIMM_WE);
	ExternalMem_Assert(SIMM_CS | SIMM_OE);
	ExternalMem_SetDataAsInput();
	ExternalMem_SetAddress(address);
	uint32_t tmp = ExternalMem_ReadData();
	//ExternalMem_DeassertOE();
	ExternalMem_Deassert(SIMM_OE);
	return tmp;
}

uint32_t ExternalMem_MaskForChips(uint8_t chips)
{
	// This is a private function we can use to
	// ignore results from chips we don't want to address
	// (or to stop from programming them)
	uint32_t mask = 0;

	if (chips & (1 << 0))
	{
		mask |= 0x000000FFUL;
	}
	if (chips & (1 << 1))
	{
		mask |= 0x0000FF00UL;
	}
	if (chips & (1 << 2))
	{
		mask |= 0x00FF0000UL;
	}
	if (chips & (1 << 3))
	{
		mask |= 0xFF000000UL;
	}

	return mask;
}

void ExternalMem_UnlockChips(uint8_t chipsMask)
{
	// Use a mask so we don't unlock chips we don't want to talk with
	uint32_t mask = ExternalMem_MaskForChips(chipsMask);

	// First part of unlock sequence:
	// Write 0x55555555 to the address bus and 0xAA to the data bus
	// (Some datasheets may only say 0x555 or 0x5555, but they ignore
	// the upper bits, so writing the alternating pattern to all address lines
	// should make it compatible with larger chips)
	ExternalMem_WriteCycle(0x55555555UL, 0xAAAAAAAAUL & mask);

	// Second part of unlock sequence is the same thing, but reversed.
	ExternalMem_WriteCycle(0xAAAAAAAAUL, 0x55555555UL & mask);
}

void ExternalMem_IdentifyChips(struct ChipID *chips)
{
	// Start by writing the unlock sequence to ALL chips
	ExternalMem_UnlockChips(ALL_CHIPS);

	// Write 0x90 to 0x55555555 for the identify command...
	ExternalMem_WriteCycle(0x55555555UL, 0x90909090UL);

	// Now we can read the vendor and product ID
	uint32_t result = ExternalMem_ReadCycle(0);

	chips[3].manufacturerID = (uint8_t)result;
	chips[2].manufacturerID = (uint8_t)(result >> 8);
	chips[1].manufacturerID = (uint8_t)(result >> 16);
	chips[0].manufacturerID = (uint8_t)(result >> 24);

	result = ExternalMem_ReadCycle(1);

	chips[3].deviceID = (uint8_t)result;
	chips[2].deviceID = (uint8_t)(result >> 8);
	chips[1].deviceID = (uint8_t)(result >> 16);
	chips[0].deviceID = (uint8_t)(result >> 24);

	// Exit software ID mode
	ExternalMem_WriteCycle(0, 0xF0F0F0F0UL);
}

void ExternalMem_EraseChips(uint8_t chipsMask)
{
	ExternalMem_UnlockChips(chipsMask);
	ExternalMem_WriteCycle(0x55555555UL, 0x80808080UL);
	ExternalMem_UnlockChips(chipsMask);
	ExternalMem_WriteCycle(0x55555555UL, 0x10101010UL);

	ExternalMem_WaitCompletion(chipsMask);
}

void ExternalMem_WaitCompletion(uint8_t chipsMask)
{
	//_delay_us(18);

	//#if 0
	// Mark the chips not requested as already completed,
	// so we don't end up waiting for them...
	// (We probably wouldn't anyway, but this is just
	// to be safe)
	uint8_t doneChipsMask = ~chipsMask & 0x0F;

	// Prime the loop...
	//union
	//{
	//	uint32_t word;
	//	uint8_t bytes[4];
	//} lastBits, tmp;

	uint32_t lastBits = ExternalMem_ReadCycle(0);

	//lastBits.word = ExternalMem_ReadCycle(0);
	while (doneChipsMask != 0x0F)
	{
/*#define TOGGLE_BIT		0x40

		tmp.word = ExternalMem_ReadCycle(0);

		// Note: The following assumes little endian byte ordering
		// (e.g. tmpBytes[0] is the least significant byte of tmpWord

		// Has this chip completed its operation? No?
		if ((doneChipsMask & (1 << 0)) == 0)
		{
			// No toggle means erase completed
			if ((tmp.bytes[0] & TOGGLE_BIT) == (lastBits.bytes[0] & TOGGLE_BIT))
			{
				doneChipsMask |= (1 << 0);
			}
		}

		if ((doneChipsMask & (1 << 1)) == 0)
		{
			// No toggle means erase completed
			if ((tmp.bytes[1] & TOGGLE_BIT) == (lastBits.bytes[1] & TOGGLE_BIT))
			{
				doneChipsMask |= (1 << 1);
			}
		}

		if ((doneChipsMask & (1 << 2)) == 0)
		{
			// No toggle means erase completed
			if ((tmp.bytes[2] & TOGGLE_BIT) == (lastBits.bytes[2] & TOGGLE_BIT))
			{
				doneChipsMask |= (1 << 2);
			}
		}

		if ((doneChipsMask & (1 << 3)) == 0)
		{
			// No toggle means erase completed
			if ((tmp.bytes[3] & TOGGLE_BIT) == (lastBits.bytes[3] & TOGGLE_BIT))
			{
				doneChipsMask |= (1 << 3);
			}
		}

		lastBits.word = tmp.word;*/

		// Compare the toggle bit to see if it didn't toggle
		uint32_t tmp = ExternalMem_ReadCycle(0);
		uint32_t mask = 0x00000040UL;
		uint8_t x;
		for (x = 0; x < 4; x++, mask <<= 8)
		{
			// No toggle means erase completed
			if ((tmp & mask) == (lastBits & mask))
			{
				doneChipsMask |= (1 << x);
			}
		}

		// Save last bits to check for toggle again
		lastBits = tmp;

		// Keep going until all four chips have gone
		// without toggling
	}
//#endif
}

void ExternalMem_WriteByteToChips(uint32_t address, uint32_t data, uint8_t chipsMask)
{
	// Use a mask so we don't unlock chips we don't want to talk with
	uint32_t mask = ExternalMem_MaskForChips(chipsMask);

	ExternalMem_UnlockChips(chipsMask);
	ExternalMem_WriteCycle(0x55555555UL, 0xA0A0A0A0UL & mask);
	ExternalMem_WriteCycle(address, data & mask);
	ExternalMem_WaitCompletion(chipsMask);
}

void ExternalMem_Write(uint32_t startAddress, uint32_t *buf, uint32_t len, uint8_t chipsMask)
{
	while (len--)
	{
		ExternalMem_WriteByteToChips(startAddress++, *buf++, chipsMask);
	}
}
