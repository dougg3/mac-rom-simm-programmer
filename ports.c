/*
 * ports.c
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2012 Doug Brown
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

#include "ports.h"
#include "mcp23s17.h"

// Save some time by not changing the register
// unless the value has changed [SPI = relatively slow]
static uint32_t savedDataDDR = 0;

void Ports_Init(void)
{
	// This module depends on the MPC23S17
	MCP23S17_Init();
	savedDataDDR = 0xFFFFFFFFUL;
	Ports_SetDataDDR(0);
}

void Ports_SetAddressOut(uint32_t data)
{
	// NOTE: If any of PORTA or PORTC or PORTD pins 0, 1, 4, 5, or 6 are set as
	// inputs, this function might mess with their pull-up resistors.
	// Only use it under normal operation when all the address pins are being
	// used as outputs

	PORTA = (data & 0xFF); // A0-A7
	PORTC = ((data >> 8) & 0xFF); // A8-A15

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (data >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	// Now, turn off the pins we have to turn off, and turn on the pins we have to turn on
	// (without affecting other pins [2, 3, and 7] that we aren't supposed to touch)
	PORTD &= (0x8C | tmp); // This should turn off all '0' bits in tmp.
	PORTD |= tmp; // This should turn on all '1' bits in tmp
}

void Ports_AddressOut_RMW(uint32_t data, uint32_t modifyMask)
{
	uint32_t modifiedDataOn = data & modifyMask;
	uint32_t modifiedDataOff = data | ~modifyMask;

	// Turn on/off requested bits in the PORT register.
	PORTA |= ((modifiedDataOn >> 0) & 0xFF);
	PORTA &= ((modifiedDataOff >> 0) & 0xFF);
	PORTC |= ((modifiedDataOn >> 8) & 0xFF);
	PORTC &= ((modifiedDataOff >> 8) & 0xFF);

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (modifiedDataOn >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	PORTD |= tmp;
	PORTD &= (0x8C | tmp);
}

void Ports_SetDataOut(uint32_t data)
{
	// NOTE: If any pins of PORTE or PORTF are set as inputs, this
	// function might mess with their pull-up resistors.
	// Only use it under normal operation when all the address pins are being
	// used as outputs

	// Set the actual outputted values
	MCP23S17_SetPins((data >> 16) & 0xFFFF); // D0-D15
	PORTE = ((data >> 8) & 0xFF); // D16-D23
	PORTF = ((data >> 0) & 0xFF); // D24-D31
}

void Ports_DataOut_RMW(uint32_t data, uint32_t modifyMask)
{
	uint32_t modifiedDataOn = data & modifyMask;
	uint32_t modifiedDataOff = data | ~modifyMask;

	// Read what's in it first...
	uint16_t outputLatches = MCP23S17_GetOutputs();
	outputLatches |= (modifiedDataOn >> 16) & 0xFFFF;
	outputLatches &= (modifiedDataOff >> 16) & 0xFFFF;
	MCP23S17_SetPins(outputLatches);

	// Turn on/off requested bits in the PORT register.
	PORTE |= ((modifiedDataOn >> 8) & 0xFF);
	PORTE &= ((modifiedDataOff >> 8) & 0xFF);
	PORTF |= ((modifiedDataOn >> 0) & 0xFF);
	PORTF &= ((modifiedDataOff >> 0) & 0xFF);
}

void Ports_SetAddressDDR(uint32_t ddr)
{
	DDRA = (ddr & 0xFF); // A0-A7
	DDRC = ((ddr >> 8) & 0xFF); // A8-A15

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (ddr >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	// Now, turn off the DDR bits we have to turn off,
	// and turn on the DDR bits we have to turn on
	// (without affecting other bits [2, 3, and 7]
	// that we aren't supposed to touch)
	DDRD &= (0x8C | tmp); // This should turn off all '0' bits in tmp.
	DDRD |= tmp; // This should turn on all '1' bits in tmp
}

void Ports_AddressDDR_RMW(uint32_t ddr, uint32_t modifyMask)
{
	uint32_t modifiedDataOn = ddr & modifyMask;
	uint32_t modifiedDataOff = ddr | ~modifyMask;

	// Turn on/off requested bits in the DDR register.
	DDRA |= ((modifiedDataOn >> 0) & 0xFF);
	DDRA &= ((modifiedDataOff >> 0) & 0xFF);
	DDRC |= ((modifiedDataOn >> 8) & 0xFF);
	DDRC &= ((modifiedDataOff >> 8) & 0xFF);

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (modifiedDataOn >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	DDRD |= tmp;
	DDRD &= (0x8C | tmp);
}

void Ports_SetDataDDR(uint32_t ddr)
{
	if (savedDataDDR != ddr)
	{
		MCP23S17_SetDDR((ddr >> 16) & 0xFFFF); // D0-D15
		DDRE = ((ddr >> 8) & 0xFF); // D16-D23
		DDRF = ((ddr >> 0) & 0xFF); // D24-D31

		savedDataDDR = ddr;
	}
}

void Ports_DataDDR_RMW(uint32_t ddr, uint32_t modifyMask)
{
	uint32_t newSavedDataDDR;
	uint32_t modifiedDataOn = ddr & modifyMask;
	uint32_t modifiedDataOff = ddr | ~modifyMask;

	// If we can get away with it, don't bother reading back...
	if (((modifyMask >> 16) & 0xFFFF) == 0xFFFF)
	{
		MCP23S17_SetDDR((modifiedDataOn >> 16) & 0xFFFF);

		// Remember what the new DDR will be
		newSavedDataDDR = (modifiedDataOn & 0xFFFF0000UL);
	}
	else // Otherwise, we have to read what's in it first...(unless I decide to keep a local cached copy)
	{
		uint16_t outputLatches = MCP23S17_GetDDR();
		outputLatches |= (modifiedDataOn >> 16) & 0xFFFF;
		outputLatches &= (modifiedDataOff >> 16) & 0xFFFF;
		MCP23S17_SetDDR(outputLatches);

		// Remember what the new DDR will be
		newSavedDataDDR = ((uint32_t)outputLatches << 16);
	}

	// Turn on/off requested bits in the DDR register.
	DDRE |= ((modifiedDataOn >> 8) & 0xFF);
	DDRE &= ((modifiedDataOff >> 8) & 0xFF);
	DDRF |= ((modifiedDataOn >> 0) & 0xFF);
	DDRF &= ((modifiedDataOff >> 0) & 0xFF);

	// Remember what the new DDR will be
	newSavedDataDDR |= ((uint32_t)DDRE) << 8;
	newSavedDataDDR |= ((uint32_t)DDRF) << 0;

	// Save the new DDR
	savedDataDDR = newSavedDataDDR;
}

void Ports_SetCSDDR(bool ddr)
{
	if (ddr)
	{
		DDRB |= SIMM_CS;
	}
	else
	{
		DDRB &= ~SIMM_CS;
	}
}

void Ports_SetOEDDR(bool ddr)
{
	if (ddr)
	{
		DDRB |= SIMM_OE;
	}
	else
	{
		DDRB &= ~SIMM_OE;
	}
}

void Ports_SetWEDDR(bool ddr)
{
	if (ddr)
	{
		DDRB |= SIMM_WE;
	}
	else
	{
		DDRB &= ~SIMM_WE;
	}
}

void Ports_AddressPullups_RMW(uint32_t pullups, uint32_t modifyMask)
{
	// Pull-ups are set by writing to the data register when in input mode.
	// MAKE SURE THE PINS ARE SET AS INPUTS FIRST!
	Ports_AddressOut_RMW(pullups, modifyMask);
}

void Ports_DataPullups_RMW(uint32_t pullups, uint32_t modifyMask)
{
	// Pull-ups here are a little more tricky because the MCP23S17 has
	// separate registers for pull-up enable.
	uint32_t modifiedDataOn = pullups & modifyMask;
	uint32_t modifiedDataOff = pullups | ~modifyMask;

	// If we can get away with it, don't bother reading back...
	if (((modifyMask >> 16) & 0xFFFF) == 0xFFFF)
	{
		MCP23S17_SetPullups((modifiedDataOn >> 16) & 0xFFFF);
	}
	else // Otherwise, we have to read what's in it first...(unless I decide to keep a local cached copy)
	{
		uint16_t outputLatches = MCP23S17_GetPullups();
		outputLatches |= (modifiedDataOn >> 16) & 0xFFFF;
		outputLatches &= (modifiedDataOff >> 16) & 0xFFFF;
		MCP23S17_SetPullups(outputLatches);
	}

	// Turn on/off requested bits in the PORT register for the other 16 bits.
	PORTE |= ((modifiedDataOn >> 8) & 0xFF);
	PORTE &= ((modifiedDataOff >> 8) & 0xFF);
	PORTF |= ((modifiedDataOn >> 0) & 0xFF);
	PORTF &= ((modifiedDataOff >> 0) & 0xFF);
}

void Ports_SetCSPullup(bool pullup)
{
	if (pullup)
	{
		PORTB |= SIMM_CS;
	}
	else
	{
		PORTB &= ~SIMM_CS;
	}
}

void Ports_SetOEPullup(bool pullup)
{
	if (pullup)
	{
		PORTB |= SIMM_OE;
	}
	else
	{
		PORTB &= ~SIMM_OE;
	}
}

void Ports_SetWEPullup(bool pullup)
{
	if (pullup)
	{
		PORTB |= SIMM_WE;
	}
	else
	{
		PORTB &= ~SIMM_WE;
	}
}


uint32_t Ports_ReadAddress(void)
{
	uint32_t result = PINA;
	result |= (((uint32_t)PINC) << 8);
	uint8_t tmp = (PIND & 0x03) | ((PIND & 0x70) >> 2);
	result |= (((uint32_t)tmp) << 16);

	return result;
}

uint32_t Ports_ReadData(void)
{
	uint32_t result = (uint32_t)MCP23S17_ReadPins() << 16;

	// Grab the other two bytes...
	result |= (((uint32_t)PINE) << 8);
	result |= (((uint32_t)PINF) << 0);

	return result;
}

bool Ports_ReadCS(void)
{
	return (PINB & SIMM_CS) != 0;
}

bool Ports_ReadOE(void)
{
	return (PINB & SIMM_OE) != 0;
}

bool Ports_ReadWE(void)
{
	return (PINB & SIMM_WE) != 0;
}
