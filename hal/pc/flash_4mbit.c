/*
 * flash_4mbit.c
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

#include "flash_4mbit.h"
#include <stdlib.h>
#include <string.h>

// TODO: Can we use a timer after an erase operation begins?
// I'd rather not base the emulation on the number of confirm read cycles
// we get...

static void UpdateGPIOPin(GPIOSimDevice *device, GPIOPin pin, bool high);
static GPIOSimValue ReadGPIOPin(GPIOSimDevice *device, GPIOPin pin);
static void HandleWriteCycle(Flash4MBit *f, uint32_t address, uint8_t data);
static void HandleReadCycle(Flash4MBit *f);
static int8_t DataPinIndex(Flash4MBit *f, GPIOPin pin);
static int8_t AddressPinIndex(Flash4MBit *f, GPIOPin pin);
static bool IsCSPin(Flash4MBit *f, GPIOPin pin);
static bool IsOEPin(Flash4MBit *f, GPIOPin pin);
static bool IsWEPin(Flash4MBit *f, GPIOPin pin);
static uint32_t UnlockAddressMask(Flash4MBit *f);
static uint8_t ManufacturerID(Flash4MBit *f);
static uint8_t DeviceID(Flash4MBit *f);
static void EraseChip(Flash4MBit *f);
static void EraseSector(Flash4MBit *f, uint32_t address);

static const GPIOSimDeviceFunctions functions = {
	.drivePin = UpdateGPIOPin,
	.readPin = ReadGPIOPin,
};

enum FlashState
{
	FlashReading,
	FlashUnlocking,
	FlashWaitingForCommand,
	FlashSoftwareID,
	FlashProgrammingByte,
	FlashEraseUnlocking1,
	FlashEraseUnlocking2,
	FlashEraseWaitingForCommand,
	FlashReadingWriteOrEraseStatus,
};

/**
 * @brief Initializes emulation for a single 4-megabit flash chip
 * @param f The flash chip object
 * @param t The type of flash chip being emulated
 */
void Flash4MBit_Init(Flash4MBit *f, Flash4MBitDeviceType t)
{
	f->base.functions = &functions;
	f->type = t;
	f->dataInput = 0;
	f->address = 0;
	f->latchedWriteAddress = 0;
	f->csAsserted = false;
	f->oeAsserted = false;
	f->weAsserted = false;
	f->softwareIDActivated = false;
	f->state = FlashReading;
	f->content = malloc(512*1024);

	// TODO: Dealloc content
}

void Flash4MBit_SetControlPins(Flash4MBit *f, GPIOPin cs, GPIOPin oe, GPIOPin we)
{
	f->csPin = cs;
	f->oePin = oe;
	f->wePin = we;
}

void Flash4MBit_SetDataPin(Flash4MBit *f, uint8_t index, GPIOPin pin)
{
	if (index < FLASH_4MBIT_DATA_PINS)
	{
		f->dataPins[index] = pin;
	}
}

void Flash4MBit_SetAddressPin(Flash4MBit *f, uint8_t index, GPIOPin pin)
{
	if (index < FLASH_4MBIT_ADDR_PINS)
	{
		f->addressPins[index] = pin;
	}
}

static void UpdateGPIOPin(GPIOSimDevice *device, GPIOPin pin, bool high)
{
	Flash4MBit *f = (Flash4MBit *)device;
	int8_t pinIndex;

	if (IsCSPin(f, pin))
	{
		if (f->csAsserted != !high)
		{
			f->csAsserted = !high;

			// If WE is asserted, latch stuff...
			if (f->weAsserted)
			{
				// Latch the address on the falling edge of CS
				if (f->csAsserted)
				{
					f->latchedWriteAddress = f->address;
				}
				// Latch the data (and update state) on the rising edge of CS
				else
				{
					HandleWriteCycle(f, f->latchedWriteAddress, f->dataInput);
				}
			}
			else if (f->oeAsserted)
			{
				if (!f->csAsserted)
				{
					// Handle a read cycle when CS is deasserted while OE is asserted.
					// The data isn't really important; it's more of a state machine update
					HandleReadCycle(f);
				}
			}
		}
	}
	else if (IsOEPin(f, pin))
	{
		if (f->oeAsserted != !high)
		{
			f->oeAsserted = !high;

			// If CS is asserted, latch stuff...
			if (f->csAsserted)
			{
				if (!f->oeAsserted)
				{
					// Handle a read cycle when OE is deasserted while CS is asserted.
					// The data isn't really important; it's more of a state machine update
					HandleReadCycle(f);
				}
			}
		}
	}
	else if (IsWEPin(f, pin))
	{
		if (f->weAsserted != !high)
		{
			f->weAsserted = !high;

			// If CS is asserted, latch stuff...
			if (f->csAsserted)
			{
				// Latch the address on the falling edge of WE
				if (f->weAsserted)
				{
					f->latchedWriteAddress = f->address;
				}
				// Latch the data (and update state) on the rising edge of WE
				else
				{
					HandleWriteCycle(f, f->latchedWriteAddress, f->dataInput);
				}
			}
		}
	}
	else if ((pinIndex = DataPinIndex(f, pin)) >= 0)
	{
		// Update the current data input state
		if (high)
		{
			f->dataInput |= (1UL << pinIndex);
		}
		else
		{
			f->dataInput &= ~(1UL << pinIndex);
		}
	}
	else if ((pinIndex = AddressPinIndex(f, pin)) >= 0)
	{
		// Update the current address
		if (high)
		{
			f->address |= (1UL << pinIndex);
		}
		else
		{
			f->address &= ~(1UL << pinIndex);
		}
	}
}

static GPIOSimValue ReadGPIOPin(GPIOSimDevice *device, GPIOPin pin)
{
	Flash4MBit *f = (Flash4MBit *)device;

	// We only read when allowed
	if (f->csAsserted && f->oeAsserted)
	{
		// Figure out if this is a data pin, that's the only pin we ever drive
		int8_t dataPinIndex = DataPinIndex(f, pin);
		if (dataPinIndex >= 0)
		{
			uint8_t dataToRead = 0;

			if (f->state == FlashSoftwareID)
			{
				// This is kind of janky, chips might have other internal registers to read. The different chip
				// types are differently picky about what bits need to be zeroed for this operation. For the purposes
				// of this emulation, just look at the bottom bit to decide manufacturer or device.
				if (f->address & 1)
				{
					dataToRead = DeviceID(f);
				}
				else
				{
					dataToRead = ManufacturerID(f);
				}
			}
			// TODO: programming toggle bit state?
			else // For example, state is FlashReading, or we're interrupting an unlock sequence
			{
				// Grab the content from the current address
				dataToRead = f->content[f->address];
			}

			// Drive high or low based on the data we're reading
			if (dataToRead & (1 << dataPinIndex))
			{
				return GPIOSimDrivingHigh;
			}
			else
			{
				return GPIOSimDrivingLow;
			}
		}
	}

	return GPIOSimNotDriving;
}

static void HandleWriteCycle(Flash4MBit *f, uint32_t address, uint8_t data)
{
	uint32_t unlockMask = UnlockAddressMask(f);
	switch (f->state)
	{
	case FlashReadingWriteOrEraseStatus:
		// Don't allow any changes to state while a write/erase is active
		break;
	case FlashReading:
	case FlashSoftwareID: // TODO: Does this state even need to exist? Or is the softwareIDActivated bool enough?
	default:
		if (((address & unlockMask) == (0x55555555UL & unlockMask)) && (data == 0xAA))
		{
			f->state = FlashUnlocking;
		}
		else if (data == 0xF0)
		{
			// Force a reset back to software ID mode
			f->softwareIDActivated = false;
			f->state = FlashReading;
		}
		break;
	case FlashUnlocking:
		if (((address & unlockMask) == (0xAAAAAAAAUL & unlockMask)) && (data == 0x55))
		{
			f->state = FlashWaitingForCommand;
		}
		else
		{
			// Invalid unlock sequence
			f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
		}
		break;
	case FlashWaitingForCommand:
		if ((address & unlockMask) == (0x55555555UL & unlockMask))
		{
			switch (data)
			{
			case 0xA0:
				f->state = FlashProgrammingByte;
				break;
			case 0x80:
				f->state = FlashEraseUnlocking1;
				break;
			case 0x90:
				f->softwareIDActivated = true;
				f->state = FlashSoftwareID;
				break;
			case 0xF0:
				f->softwareIDActivated = false;
				f->state = FlashReading;
				break;
			default:
				// Invalid command, go back to the read state we should be in
				f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
				break;
			}
		}
		else
		{
			// Invalid unlock sequence
			f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
		}
		break;
	case FlashProgrammingByte:
		// Write the data, then begin reading back the status
		// Programming is only capable of turning 1 bits into zero bits.
		// In other words, programming takes the current byte stored in the flash
		// and ANDs it with the value you're trying to program to that byte.
		// The resulting value is what gets programmed to the chip.
		f->content[address & 0x7FFFF] &= data;
		/// TODO: Set up timer for erase?
		f->state = FlashReadingWriteOrEraseStatus;
		break;
	case FlashEraseUnlocking1:
		if (((address & unlockMask) == (0x55555555UL & unlockMask)) && (data == 0xAA))
		{
			f->state = FlashEraseUnlocking2;
		}
		else
		{
			f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
		}
		break;
	case FlashEraseUnlocking2:
		if (((address & unlockMask) == (0xAAAAAAAAUL & unlockMask)) && (data == 0x55))
		{
			f->state = FlashEraseWaitingForCommand;
		}
		else
		{
			// Invalid unlock sequence
			f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
		}
		break;
	case FlashEraseWaitingForCommand:
		if (((address & unlockMask) == (0x55555555UL & unlockMask)) && data == 0x10)
		{
			// Erase the entire chip
			EraseChip(f);
			/// TODO: Set up timer for erase?
			f->state = FlashReadingWriteOrEraseStatus;
		}
		else if (data == 0x30)
		{
			EraseSector(f, address);
			/// TODO: Set up timer for erase?
			f->state = FlashReadingWriteOrEraseStatus;
		}
		else
		{
			// Invalid command/address supplied during erase
			f->state = f->softwareIDActivated ? FlashSoftwareID : FlashReading;
		}
		break;
	}
}

static void HandleReadCycle(Flash4MBit *f)
{
	// We don't care about the address here because most of the output logic is handled in the ReadGPIOPin handler.
	// This just makes sure the state machine is up to date after a read cycle.
}

static int8_t DataPinIndex(Flash4MBit *f, GPIOPin pin)
{
	for (int i = 0; i < FLASH_4MBIT_DATA_PINS; i++)
	{
		if (f->dataPins[i].port == pin.port &&
			f->dataPins[i].pin == pin.pin)
		{
			return i;
		}
	}
	return -1;
}

static int8_t AddressPinIndex(Flash4MBit *f, GPIOPin pin)
{
	for (int i = 0; i < FLASH_4MBIT_ADDR_PINS; i++)
	{
		if (f->addressPins[i].port == pin.port &&
			f->addressPins[i].pin == pin.pin)
		{
			return i;
		}
	}
	return -1;
}

static bool IsCSPin(Flash4MBit *f, GPIOPin pin)
{
	return f->csPin.port == pin.port && f->csPin.pin == pin.pin;
}

static bool IsOEPin(Flash4MBit *f, GPIOPin pin)
{
	return f->oePin.port == pin.port && f->oePin.pin == pin.pin;
}

static bool IsWEPin(Flash4MBit *f, GPIOPin pin)
{
	return f->wePin.port == pin.port && f->wePin.pin == pin.pin;
}

static uint32_t UnlockAddressMask(Flash4MBit *f)
{
	switch (f->type)
	{
	case Flash_SST39SF040:
		// SST39SF040 only cares about A0 through A14 (low 15 bits)
		return 0x7FFFUL;
	case Flash_AM29F040B:
		// AM329F040B only cares about A0 through A10 (low 11 bits)
		return 0x7FFUL;
	default:
		// Default behavior is we care about all 19 address bits during unlock operations
		return 0x7FFFFUL;
	}
}

static uint8_t ManufacturerID(Flash4MBit *f)
{
	switch (f->type)
	{
	case Flash_SST39SF040:
		return 0xBF;
	case Flash_AM29F040B:
		return 0x01;
	default:
		return 0;
	}
}

static uint8_t DeviceID(Flash4MBit *f)
{
	switch (f->type)
	{
	case Flash_SST39SF040:
		return 0xB7;
	case Flash_AM29F040B:
		return 0xA4;
	default:
		return 0;
	}
}
static void EraseChip(Flash4MBit *f)
{
	memset(f->content, 0xFF, 512*1024);
}

static void EraseSector(Flash4MBit *f, uint32_t address)
{
	uint32_t sectorSize;

	switch (f->type)
	{
	case Flash_SST39SF040:
	default:
		// 4 KB sectors
		sectorSize = 4 * 1024;
		break;
	case Flash_AM29F040B:
		// 64 KB sectors
		sectorSize = 64 * 1024;
		break;
	}

	// Mask out the low bits of the address so we have the sector start address.
	// We're guaranteed that sectorSize is a power of 2, so subtracting 1 will
	// give us a mask we can use to turn off address bits to get the start addr.
	address &= ~(sectorSize - 1);
	// Clear those bytes
	memset(f->content + address, 0xFF, sectorSize);
}
