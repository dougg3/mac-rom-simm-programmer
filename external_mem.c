/*
 * external_mem.c
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#include "external_mem.h"
#include "mcp23s17.h"
#include <avr/io.h>

// SIMM control signals on port B
#define SIMM_WE		(1 << 6)
#define SIMM_OE		(1 << 5)
#define SIMM_CS		(1 << 4)

static bool ExternalMem_Inited = false;

void ExternalMem_Init(void)
{
	// If it has already been initialized, no need to do it again.
	if (ExternalMem_Inited)
	{
		return;
	}

	// This module depends on the MCP23S17
	MCP23S17_Init();

	// Configure address lines as outputs
	DDRA = 0xFF; // A0-A7
	DDRC = 0xFF; // A8-A15
	DDRD |= 0x73; // A16-A20

	// Sensible defaults for address and data lines
	ExternalMem_SetAddress(0);
	ExternalMem_SetDataAsInput();

	// Control lines
	DDRB |= SIMM_WE | SIMM_OE | SIMM_CS;

	// Default all the control lines to high (de-asserted)
	PORTB |= SIMM_WE | SIMM_OE | SIMM_CS;

	// All done!
	ExternalMem_Inited = true;
}

void ExternalMem_SetAddress(uint32_t address)
{
	PORTA = (address & 0xFF); // A0-A7
	PORTC = ((address >> 8) & 0xFF); // A8-A15

	// A16-A20 are special because they are split up...(We use PORTD pins 0, 1, 4, 5, 6)
	uint8_t tmp = (address >> 16) & 0xFF;
	tmp = (tmp & 0x03) | ((tmp & 0x1C) << 2);

	// Now, turn off the pins we have to turn off, and turn on the pins we have to turn on
	// (without affecting other pins [2, 3, and 7] that we aren't supposed to touch)
	PORTD &= (0x8C | tmp); // This should turn off all '0' bits in tmp.
	PORTD |= tmp; // This should turn on all '1' bits in tmp
}

void ExternalMem_SetData(uint32_t data)
{
	// Set as outputs
	MCP23S17_SetDDR(0xFFFF); // D0-D15
	DDRE = 0xFF; // D16-D23
	DDRF = 0xFF; // D24-D31

	// Set the actual outputted values
	MCP23S17_SetPins(data & 0xFFFF); // D0-D15
	PORTE = ((data >> 16) & 0xFF); // D16-D23
	PORTF = ((data >> 24) & 0xFF); // D24-D31
}

void ExternalMem_SetAddressAndData(uint32_t address, uint32_t data)
{
	ExternalMem_SetAddress(address);
	ExternalMem_SetData(data);
}

void ExternalMem_SetDataAsInput(void)
{
	MCP23S17_SetDDR(0x0000); // D0-D15
	DDRE = 0; // D16-D23
	DDRF = 0; // D24-D31
}

uint32_t ExternalMem_ReadData(void)
{
	return ((uint32_t)MCP23S17_ReadPins()) |
			(((uint32_t)PORTE) << 16) |
			(((uint32_t)PORTF) << 24);
}

void ExternalMem_AssertCS(void)
{
	PORTB |= SIMM_CS;
}

void ExternalMem_DeassertCS(void)
{
	PORTB &= ~SIMM_CS;
}

void ExternalMem_AssertWE(void)
{
	PORTB |= SIMM_WE;
}

void ExternalMem_DeassertWE(void)
{
	PORTB &= ~SIMM_WE;
}

void ExternalMem_AssertOE(void)
{
	PORTB |= SIMM_OE;
}

void ExternalMem_DeassertOE(void)
{
	PORTB &= ~SIMM_OE;
}
