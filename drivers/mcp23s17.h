/*
 * mcp23s17.h
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2020 Doug Brown
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

#ifndef DRIVERS_MCP23S17_H_
#define DRIVERS_MCP23S17_H_

#include "../hal/spi.h"

/// A few register defines. Ordinarily I wouldn't put these in the header file
/// because users of this module shouldn't care...but I have had to do some
/// optimizations with the AVR to bypass the hardware abstraction layer.
#define MCP23S17_CONTROL_WRITE(address)		(0x40 | (address << 1))
#define MCP23S17_CONTROL_READ(address)		(0x40 | (address << 1) | 1)
#define MCP23S17_IODIRA						0x00
#define MCP23S17_IODIRB						0x01
#define MCP23S17_IPOLA						0x02
#define MCP23S17_IPOLB						0x03
#define MCP23S17_GPINTENA					0x04
#define MCP23S17_GPINTENB					0x05
#define MCP23S17_DEFVALA					0x06
#define MCP23S17_DEFVALB					0x07
#define MCP23S17_INTCONA					0x08
#define MCP23S17_INTCONB					0x09
#define MCP23S17_IOCON						0x0A
#define MCP23S17_IOCON_AGAIN				0x0B
#define MCP23S17_GPPUA						0x0C
#define MCP23S17_GPPUB						0x0D
#define MCP23S17_INTFA						0x0E
#define MCP23S17_INTFB						0x0F
#define MCP23S17_INTCAPA					0x10
#define MCP23S17_INTCAPB					0x11
#define MCP23S17_GPIOA						0x12
#define MCP23S17_GPIOB						0x13
#define MCP23S17_OLATA						0x14
#define MCP23S17_OLATB						0x15

/// Struct representing a single MCP23S17 device
typedef struct MCP23S17
{
	/// The SPI device representing this MCP23S17
	SPIDevice spi;
} MCP23S17;

void MCP23S17_Init(MCP23S17 *mcp, GPIOPin resetPin);
void MCP23S17_Begin(MCP23S17 *mcp);
void MCP23S17_End(MCP23S17 *mcp);
void MCP23S17_SetDDR(MCP23S17 *mcp, uint16_t ddr);
uint16_t MCP23S17_DDR(MCP23S17 *mcp);
void MCP23S17_SetOutputs(MCP23S17 *mcp, uint16_t data);
uint16_t MCP23S17_Outputs(MCP23S17 *mcp);
uint16_t MCP23S17_ReadInputs(MCP23S17 *mcp);
void MCP23S17_SetPullups(MCP23S17 *mcp, uint16_t pullups);
uint16_t MCP23S17_Pullups(MCP23S17 *mcp);

#endif /* DRIVERS_MCP23S17_H_ */
