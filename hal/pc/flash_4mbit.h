/*
 * flash_4mbit.h
 *
 *  Created on: Jul 27, 2021
 *      Author: Doug
 *
 * Copyright (C) 2011-2021 Doug Brown
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

#ifndef FLASH_4MBIT_H
#define FLASH_4MBIT_H

#include "gpio_sim.h"

#define FLASH_4MBIT_DATA_PINS					8
#define FLASH_4MBIT_ADDR_PINS					19

/// Enum representing the type of 4-megabit flash chip being emulated
typedef enum Flash4MBitDeviceType
{
	/// SST/Microchip SST39SF040 4-megabit flash chip
	Flash_SST39SF040,
	/// AMD/Infineon AM29F040B 4-megabit flash chips
	Flash_AM29F040B,
} Flash4MBitDeviceType;

/// Implementation of GPIOSimDevice for a 4-megabit flash chip
typedef struct Flash4MBit
{
	/// Base class
	GPIOSimDevice base;
	/// The type of flash being emulated
	Flash4MBitDeviceType type;
	/// Current data input state
	uint8_t dataInput;
	/// Current address
	uint32_t address;
	/// The write address that has been latched
	uint32_t latchedWriteAddress;
	/// True if CS is asserted
	bool csAsserted;
	/// True if OE is asserted
	bool oeAsserted;
	/// True if WE is asserted
	bool weAsserted;
	/// True if we've activated software ID mode
	bool softwareIDActivated;
	/// All of our pins
	GPIOPin csPin;
	GPIOPin oePin;
	GPIOPin wePin;
	GPIOPin dataPins[FLASH_4MBIT_DATA_PINS];
	GPIOPin addressPins[FLASH_4MBIT_ADDR_PINS];
	/// The state -- private so we store as an 8-bit int rather than the enum
	uint8_t state;
	uint8_t *content;
} Flash4MBit;

void Flash4MBit_Init(Flash4MBit *f, Flash4MBitDeviceType t);
void Flash4MBit_SetControlPins(Flash4MBit *f, GPIOPin cs, GPIOPin oe, GPIOPin we);
void Flash4MBit_SetDataPin(Flash4MBit *f, uint8_t index, GPIOPin pin);
void Flash4MBit_SetAddressPin(Flash4MBit *f, uint8_t index, GPIOPin pin);

#endif // FLASH_4MBIT_H
