/*
 * mcp23s17.h
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#ifndef MCP23S17_H_
#define MCP23S17_H_

#include <stdint.h>

void MCP23S17_Init();
void MCP23S17_SetDDR(uint16_t ddr);
void MCP23S17_SetPins(uint16_t data);
uint16_t MCP23S17_ReadPins(void);

#endif /* MCP23S17_H_ */
