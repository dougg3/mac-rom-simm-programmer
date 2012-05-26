/*
 * mcp23s17.h
 *
 *  Created on: Nov 25, 2011
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

#ifndef MCP23S17_H_
#define MCP23S17_H_

#include <stdint.h>

void MCP23S17_Init();
void MCP23S17_SetDDR(uint16_t ddr);
void MCP23S17_SetPins(uint16_t data);
uint16_t MCP23S17_ReadPins(void);
void MCP23S17_SetPullups(uint16_t pullups);
uint16_t MCP23S17_GetOutputs(void);
uint16_t MCP23S17_GetDDR(void);
uint16_t MCP23S17_GetPullups(void);

#endif /* MCP23S17_H_ */
