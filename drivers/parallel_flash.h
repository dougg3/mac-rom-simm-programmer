/*
 * parallel_flash.h
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

#ifndef DRIVERS_PARALLEL_FLASH_H_
#define DRIVERS_PARALLEL_FLASH_H_

#include "../hal/parallel_bus.h"

/// The number of chips we are simultaneously addressing
#define PARALLEL_FLASH_NUM_CHIPS		4

/// Masks for functions that want a chip mask...
#define IC1				(1 << 3)
#define IC2				(1 << 2)
#define IC3				(1 << 1)
#define IC4				(1 << 0)
#define ALL_CHIPS		(IC1 | IC2 | IC3 | IC4)

/// Holds info about the chip (retrieved with JEDEC standards)
typedef struct ParallelFlashChipID
{
	/// The manufacturer ID
	uint8_t manufacturer;
	/// The device ID
	uint8_t device;
} ParallelFlashChipID;

/// Type/layout of chips currently being addressed
typedef enum ParallelFlashChipType
{
	/// Four SST39SF040 chips, 512 KB each, for a total of 2 MB
	ParallelFlash_SST39SF040_x4,
	/// Four M29F160FB5AN6E2 chips, 2 MB each, in 8-bit mode, for a total of 8 MB
	ParallelFlash_M29F160FB5AN6E2_x4,
} ParallelFlashChipType;

// Tells which type of flash chip we are communicating with
void ParallelFlash_SetChipType(ParallelFlashChipType type);
ParallelFlashChipType ParallelFlash_ChipType(void);

// Reads a set of data from all 4 chips simultaneously
void ParallelFlash_Read(uint32_t startAddress, uint32_t *buf, uint16_t len);

// Does an unlock sequence on the chips requested
void ParallelFlash_UnlockChips(uint8_t chipsMask);

// Identifies all four chips
void ParallelFlash_IdentifyChips(ParallelFlashChipID *chips);

// Erases the chips/sectors requested
void ParallelFlash_EraseChips(uint8_t chipsMask);
bool ParallelFlash_EraseSectors(uint32_t address, uint32_t length, uint8_t chipsMask);

// Writes a buffer to all 4 chips simultaneously (each uint32_t contains an 8-bit portion for each chip).
// Optimized variant of this function if we know we're writing to all 4 chips simultaneously.
// Allows us to bypass a lot of operations involving "chipsMask".
void ParallelFlash_WriteAllChips(uint32_t startAddress, uint32_t const *buf, uint16_t len);

// Writes a buffer to a mask of requested chips (each uint32_t contains an 8-bit portion for each chip).
void ParallelFlash_WriteSomeChips(uint32_t startAddress, uint32_t const *buf, uint16_t len, uint8_t chipsMask);

#endif /* DRIVERS_PARALLEL_FLASH_H_ */
