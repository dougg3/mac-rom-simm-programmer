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

// Holds info about the chip (retrieved with JEDEC standards)
struct ChipID
{
	uint8_t manufacturerID;
	uint8_t deviceID;
};

// Masks for functions that want a chip...
#define IC1				(1 << 3)
#define IC2				(1 << 2)
#define IC3				(1 << 1)
#define IC4				(1 << 0)
#define ALL_CHIPS		(IC1 | IC2 | IC3 | IC4)

// I feel kind of sick making these available to the outside world, but
// I'm doing it for efficiency.
// These are the bits on port B corresponding to the control signals.
// Pass them to ExternalMem_Assert() or ExternalMem_Deassert().
#define SIMM_WE		(1 << 6)
#define SIMM_OE		(1 << 5)
#define SIMM_CS		(1 << 4)

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

/*// Sets the state of the chip select line
void ExternalMem_AssertCS(void);
void ExternalMem_DeassertCS(void);

// Sets the state of the write enable line
void ExternalMem_AssertWE(void);
void ExternalMem_DeassertWE(void);

// Sets the state of the output enable line
void ExternalMem_AssertOE(void);
void ExternalMem_DeassertOE(void);*/

// This is not the nicest-looking software engineering practice
// in the world, but it saves needlessly wasted CPU cycles
// that would be wasted in layers upon layers of abstraction
#define ExternalMem_Assert(assertMask) PORTB &= ~(assertMask);
#define ExternalMem_Deassert(assertMask) PORTB |= (assertMask);

// Reads a set of data from all 4 chips simultaneously
void ExternalMem_Read(uint32_t startAddress, uint32_t *buf, uint32_t len);

// Performs a single write cycle on all 4 chips simultaneously
void ExternalMem_WriteCycle(uint32_t address, uint32_t data);

// Performs a single read cycle on all 4 chips simultaneously
uint32_t ExternalMem_ReadCycle(uint32_t address);

// Does an unlock sequence on the chips requested
void ExternalMem_UnlockChips(uint8_t chipsMask);

// Identifies the chips that are currently in the SIMM
void ExternalMem_IdentifyChips(struct ChipID *chips);

// Erases the chips requested
void ExternalMem_EraseChips(uint8_t chipsMask);

// Writes a byte to the chips requested at the specified address
void ExternalMem_WriteByteToChips(uint32_t address, uint32_t data, uint8_t chipsMask);

// Writes a buffer to the requested chips simultaneously
// (each uint32_t contains an 8-bit portion for each chip,
// which is masked away if the chip is not requested)
void ExternalMem_Write(uint32_t startAddress, uint32_t *buf, uint32_t len, uint8_t chipsMask);

#endif /* EXTERNAL_MEM_H_ */
