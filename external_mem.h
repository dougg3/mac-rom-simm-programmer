/*
 * external_mem.h
 *
 *  Created on: Nov 25, 2011
 *      Author: Doug
 */

#ifndef EXTERNAL_MEM_H_
#define EXTERNAL_MEM_H_

#include <stdint.h>
#include <stdbool.h>

// Initializes the (bit-banged) external memory interface
void ExternalMem_Init(void);

// Sets the value outputted to the address lines
void ExternalMem_SetAddress(uint32_t address);

// Sets the value outputted to the data lines
void ExternalMem_SetData(uint32_t data);

// Sets the value outputted to the address and data lines
void ExternalMem_SetAddressAndData(uint32_t address, uint32_t data);

// Puts the data lines into input mode, for reading back stored data
void ExternalMem_SetDataAsInput(void);

// Reads back the value the chips are putting onto the data lines
uint32_t ExternalMem_ReadData(void);

// Sets the state of the chip select line
void ExternalMem_AssertCS(void);
void ExternalMem_DeassertCS(void);

// Sets the state of the write enable line
void ExternalMem_AssertWE(void);
void ExternalMem_DeassertWE(void);

// Sets the state of the output enable line
void ExternalMem_AssertOE(void);
void ExternalMem_DeassertOE(void);

// Reads a set of data...
void ExternalMem_Read(uint32_t startAddress, uint32_t *buf, uint32_t len);

#endif /* EXTERNAL_MEM_H_ */
