/*
 * parallel_flash.c
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
 */

#include "parallel_flash.h"
#include "../util.h"

/// Erasable sector size in SST39SF040
#define SECTOR_SIZE_SST39SF040			(4*1024UL)
/// Erasable sector size in M29F160FB5AN6E2, 8-bit mode
#define SECTOR_SIZE_M29F160FB5AN6E2_8	(64*1024UL)

static uint32_t ParallelFlash_MaskForChips(uint8_t chips);
static ALWAYS_INLINE void ParallelFlash_WaitForCompletion(void);
static ALWAYS_INLINE uint32_t ParallelFlash_UnlockAddress1(void);

/// The type/arrangement of parallel flash chips we are talking to
static ParallelFlashChipType curChipType = ParallelFlash_SST39SF040_x4;

/** Sets the type/arrangement of parallel flash chips we are talking to
 *
 * @param type The type/arrangement of flash chips
 */
void ParallelFlash_SetChipType(ParallelFlashChipType type)
{
	curChipType = type;
}

/** Gets the type/arrangement of parallel flash chips we are talking to
 *
 * @return The current type/arrangement of flash chips
 */
ParallelFlashChipType ParallelFlash_ChipType(void)
{
	return curChipType;
}

/** Reads data from the flash chip
 *
 * @param startAddress The address for reading
 * @param buf The buffer to read to
 * @param len The number of bytes to read
 */
void ParallelFlash_Read(uint32_t startAddress, uint32_t *buf, uint16_t len)
{
	// Just forward this request directly onto the parallel bus. Nothing
	// special is required for reading a chunk of data.
	ParallelBus_Read(startAddress, buf, len);
}

/** Unlocks the flash chips using the special write sequence
 *
 * @param chipsMask The mask of which chips to unlock
 */
void ParallelFlash_UnlockChips(uint8_t chipsMask)
{
	// Use a mask so we don't unlock chips we don't want to talk with
	uint32_t mask = ParallelFlash_MaskForChips(chipsMask);
	uint32_t unlockAddress = ParallelFlash_UnlockAddress1();

	// First part of unlock sequence:
	// Write 0x55555555 to the address bus and 0xAA to the data bus
	// (Some datasheets may only say 0x555 or 0x5555, but they ignore
	// the upper bits, so writing the alternating pattern to all address lines
	// should make it compatible with larger chips).
	ParallelBus_WriteCycle(unlockAddress, 0xAAAAAAAAUL & mask);

	// Second part of unlock sequence is the same thing, but reversed.
	ParallelBus_WriteCycle(~unlockAddress, 0x55555555UL & mask);
}

/** Reads the ID of the chips
 *
 * @param Pointer to variable for storing ID info about each chip
 */
void ParallelFlash_IdentifyChips(ParallelFlashChipID *chips)
{
	// Start by writing the unlock sequence to ALL chips
	ParallelFlash_UnlockChips(ALL_CHIPS);

	// Write 0x90 to the first unlock address for the identify command...
	ParallelBus_WriteCycle(ParallelFlash_UnlockAddress1(), 0x90909090UL);

	// Now we can read the vendor and product ID from addresses 0 and 1
	// (or 1 and 2 if we're using the M29F160FB5AN6E2 in 8-bit mode).
	// Note: The Micron datasheet says it requires 12V to be applied to A9
	// in order for the identification command to work properly, but in
	// practice the identification process works fine without it.
	uint32_t vendorAddress = curChipType != ParallelFlash_M29F160FB5AN6E2_x4 ?
			0 : 1;
	uint32_t manufacturers = ParallelBus_ReadCycle(vendorAddress);
	uint32_t devices = ParallelBus_ReadCycle(vendorAddress + 1);
	for (int8_t i = 0; i < PARALLEL_FLASH_NUM_CHIPS; i++)
	{
		uint8_t manufacturer = (uint8_t)(manufacturers >> (8 * i));
		uint8_t device = (uint8_t)(devices >> (8 * i));
		chips[PARALLEL_FLASH_NUM_CHIPS - i - 1].manufacturer = manufacturer;
		chips[PARALLEL_FLASH_NUM_CHIPS - i - 1].device = device;
	}

	// Exit software ID mode
	ParallelBus_WriteCycle(0, 0xF0F0F0F0UL);
}

/** Erases the specified chips
 *
 * @param chipsMask The mask of which chips to erase
 */
void ParallelFlash_EraseChips(uint8_t chipsMask)
{
	uint32_t unlockAddress = ParallelFlash_UnlockAddress1();
	ParallelFlash_UnlockChips(chipsMask);
	ParallelBus_WriteCycle(unlockAddress, 0x80808080UL);
	ParallelFlash_UnlockChips(chipsMask);
	ParallelBus_WriteCycle(unlockAddress, 0x10101010UL);
	ParallelFlash_WaitForCompletion();
}

/** Erases only the range of sectors specified in the specified chips
 *
 * @param address The start address to erase (must be aligned to a sector boundary)
 * @param length The number of bytes to erase (must be aligned to a sector boundary)
 * @param chipsMask The mask of which chips to erase
 * @return True on success, false on failure
 */
bool ParallelFlash_EraseSectors(uint32_t address, uint32_t length, uint8_t chipsMask)
{
	bool result = false;

	// Figure out our sector size
	uint32_t sectorSize;
	switch (curChipType)
	{
	case ParallelFlash_SST39SF040_x4:
	default:
		sectorSize = SECTOR_SIZE_SST39SF040;
		break;
	case ParallelFlash_M29F160FB5AN6E2_x4:
		sectorSize = SECTOR_SIZE_M29F160FB5AN6E2_8;
		break;
	}

	// Make sure the area requested to be erased is on good boundaries
	if ((address % sectorSize) ||
		(length % sectorSize))
	{
		return false;
	}

	// We're good to go. Let's do it. The process varies based on the chip type
	if (curChipType == ParallelFlash_SST39SF040_x4)
	{
		// This chip sucks because you have to erase each sector with its own
		// complete erase unlock command, which can take a while. At least
		// individual erase operations are much faster on this chip...
		while (length)
		{
			// Start the erase command
			ParallelFlash_UnlockChips(chipsMask);
			ParallelBus_WriteCycle(ParallelFlash_UnlockAddress1(), 0x80808080UL);
			ParallelFlash_UnlockChips(chipsMask);

			// Now provide a sector address, but only one. Then the whole
			// unlock sequence has to be done again after this sector is done.
			ParallelBus_WriteCycle(address, 0x30303030UL);

			address += sectorSize;
			length -= sectorSize;

			// Wait for completion of this individual erase operation before
			// we can start a new erase operation.
			ParallelFlash_WaitForCompletion();
		}

		result = true;
	}
	else if (curChipType == ParallelFlash_M29F160FB5AN6E2_x4)
	{
		// This chip is nicer because it can take all the sector addresses at
		// once and then do the final erase operation in one fell swoop.
		// Start the erase command
		ParallelFlash_UnlockChips(chipsMask);
		ParallelBus_WriteCycle(ParallelFlash_UnlockAddress1(), 0x80808080UL);
		ParallelFlash_UnlockChips(chipsMask);

		// Now provide as many sector addresses as needed to erase.
		// The first address is a bit of a special case because the boot sector
		// actually has finer granularity for sector sizes.
		if (address == 0)
		{
			ParallelBus_WriteCycle(0x00000000UL, 0x30303030UL);
			ParallelBus_WriteCycle(0x00004000UL, 0x30303030UL);
			ParallelBus_WriteCycle(0x00006000UL, 0x30303030UL);
			ParallelBus_WriteCycle(0x00008000UL, 0x30303030UL);
			address += sectorSize;
			length -= sectorSize;
		}

		// The remaining sectors can use a more generic algorithm
		while (length)
		{
			ParallelBus_WriteCycle(address, 0x30303030UL);
			address += sectorSize;
			length -= sectorSize;
		}

		// Wait for completion of the entire erase operation
		ParallelFlash_WaitForCompletion();

		result = true;
	}

	return result;

}

/** Writes a buffer of data to all 4 chips simultaneously
 *
 * @param startAddress The starting address to write in flash
 * @param buf The buffer to write
 * @param len The length of data to write
 *
 * The API may look silly to have broken into different functions like this, but
 * it's a performance optimization. It means we don't have to check during every
 * byte write to see the chip unlock mask. It saves a bunch of time.
 */
void ParallelFlash_WriteAllChips(uint32_t startAddress, uint32_t const *buf, uint16_t len)
{
	uint32_t unlockAddress = ParallelFlash_UnlockAddress1();

	// Normal write process used by most parallel flashes
	if (curChipType != ParallelFlash_M29F160FB5AN6E2_x4)
	{
		while (len--)
		{
			// Write this byte.
			// Unlock...and don't use the unlock function because this one
			// is more efficient knowing the mask is 0xFFFFFFFF
			ParallelBus_WriteCycle(unlockAddress, 0xAAAAAAAAUL);
			ParallelBus_WriteCycle(~unlockAddress, 0x55555555UL);
			ParallelBus_WriteCycle(unlockAddress, 0xA0A0A0A0UL);
			ParallelBus_WriteCycle(startAddress, *buf);
			ParallelFlash_WaitForCompletion();

			startAddress++;
			buf++;
		}
	}
	// Optimized write process available on the M29F160FB5AN6E2, requires
	// fewer write cycles per byte if you know you're writing multiple bytes.
	else
	{
		// Do an unlock bypass command so that we can write bytes faster.
		// Writes will only require 2 write cycles instead of 4
		ParallelBus_WriteCycle(unlockAddress, 0xAAAAAAAAUL);
		ParallelBus_WriteCycle(~unlockAddress, 0x55555555UL);
		ParallelBus_WriteCycle(unlockAddress, 0x20202020UL);

		while (len--)
		{
			// Write this byte.
			ParallelBus_WriteCycle(0, 0xA0A0A0A0UL);
			ParallelBus_WriteCycle(startAddress, *buf);
			ParallelFlash_WaitForCompletion();

			startAddress++;
			buf++;
		}

		// When we're all done, do "unlock bypass reset" to exit from
		// programming mode
		ParallelBus_WriteCycle(0, 0x90909090UL);
		ParallelBus_WriteCycle(0, 0x00000000UL);
	}
}

/** Writes a buffer of data to the specified chips simultaneously
 *
 * @param startAddress The starting address to write in flash
 * @param buf The buffer to write
 * @param len The length of data to write
 * @param chipsMask The mask of which chips to write
 */
void ParallelFlash_WriteSomeChips(uint32_t startAddress, uint32_t const *buf, uint16_t len, uint8_t chipsMask)
{
	uint32_t unlockAddress = ParallelFlash_UnlockAddress1();

	// Normal write process used by most parallel flashes
	if (curChipType != ParallelFlash_M29F160FB5AN6E2_x4)
	{
		while (len--)
		{
			// Write this byte.
			ParallelFlash_UnlockChips(chipsMask);
			ParallelBus_WriteCycle(unlockAddress, 0xA0A0A0A0UL);
			ParallelBus_WriteCycle(startAddress, *buf);
			ParallelFlash_WaitForCompletion();

			startAddress++;
			buf++;
		}
	}
	// Optimized write process available on the M29F160FB5AN6E2, requires
	// fewer write cycles per byte if you know you're writing multiple bytes.
	else
	{
		// Do an unlock bypass command so that we can write bytes faster.
		// Writes will only require 2 write cycles instead of 4
		ParallelFlash_UnlockChips(chipsMask);
		ParallelBus_WriteCycle(unlockAddress, 0x20202020UL);

		while (len--)
		{
			// Write this byte.
			ParallelBus_WriteCycle(0, 0xA0A0A0A0UL);
			ParallelBus_WriteCycle(startAddress, *buf);
			ParallelFlash_WaitForCompletion();

			startAddress++;
			buf++;
		}

		// When we're all done, do "unlock bypass reset" to exit from
		// programming mode
		ParallelBus_WriteCycle(0, 0x90909090UL);
		ParallelBus_WriteCycle(0, 0x00000000UL);
	}
}

/** Calculates a 32-bit mask to use with the unlock process when unlocking chips
 *
 * @param chipsMask The mask of which chips to write
 * @return A 32-bit mask that can be used on the data bus to filter out the unlock sequence
 *
 * For clarity, chipsMask has 1 bit per chip. The return value has 1 byte per
 * chip. The return value masks the unlock sequence so we only supply a valid
 * unlock sequence to the chips that we want to unlock.
 */
static uint32_t ParallelFlash_MaskForChips(uint8_t chips)
{
	// Calculates a mask so we can filter out chips we don't care about.

	// Optimization because we typically don't mask the chips
	if (chips == 0x0F)
	{
		return 0xFFFFFFFFUL;
	}

	// This probably looks dumb not doing this as a loop...but AVR GCC is
	// terrible. This approach results in more optimal generated instructions.
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

/** Waits for an erase or write operation on the flash chip to complete.
 *
 * We know we're done when the value we read from the chip stops changing. There
 * is a "toggle" status bit that will stop toggling when the op is complete.
 */
static ALWAYS_INLINE void ParallelFlash_WaitForCompletion(void)
{
	uint32_t readback = ParallelBus_ReadCycle(0);
	uint32_t next = ParallelBus_ReadCycle(0);
	while (next != readback)
	{
		readback = next;
		next = ParallelBus_ReadCycle(0);
	}
}

/** Gets the first unlock address to use when unlocking writes on this chip
 *
 * @return The first unlock address.
 *
 * Note: The second unlock address is the bitwise NOT of this address.
 */
static ALWAYS_INLINE uint32_t ParallelFlash_UnlockAddress1(void)
{
	// Most chips use alternating bits, with A0 being a 1 bit
	if (curChipType != ParallelFlash_M29F160FB5AN6E2_x4)
	{
		return 0x55555555UL;
	}
	// The M29F160FB5AN6E2 is weird because it's an 8-/16-bit chip. In 8-bit
	// mode it has an "A-1" pin that we treat as A0, then the chip's A0 pin is
	// really our A1, and so on. The unlock sequence still starts on the chip's
	// physical pin A0, so effectively the unlock address is inverted.
	else
	{
		return 0xAAAAAAAAUL;
	}
}
