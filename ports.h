/*
 * ports.h
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 */

#ifndef PORTS_H_
#define PORTS_H_

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

// Under normal operation, we will only be using the address pins as outputs.
// The data pins will switch back and forth between all inputs and all outputs.
// The CS/OE/WE pins will always be outputs.
// The pullups will all be turned off.
// So you should use Ports_SetAddressOut(),
// Ports_SetDataOut(),
// Ports_SetAddressDDR() [once at the beginning]
// Ports_SetDataDDR(),
// and Ports_ReadData
//
// The reason I have implemented all this functionality is to give me complete
// control over all the pins for other use cases, such as a SIMM electrical test.
// By playing with pull-ups and inputs and outputs, I should be able to detect
// many shorted output/input scenarios. So even though these functions are overkill,
// they will be useful for diagnostics.

// I feel kind of sick making these available to the outside world, but
// I'm doing it for efficiency because they change fairly often.
// These are the bits on port B corresponding to the control signals.
#define SIMM_WE		(1 << 6)
#define SIMM_OE		(1 << 5)
#define SIMM_CS		(1 << 4)

void Ports_Init(void);

void Ports_SetAddressOut(uint32_t data);
void Ports_AddressOut_RMW(uint32_t data, uint32_t modifyMask);
void Ports_SetDataOut(uint32_t data);
void Ports_DataOut_RMW(uint32_t data, uint32_t modifyMask);
#define Ports_SetCSOut(data) do { if (data) Ports_ControlOn(SIMM_CS); else Ports_ControlOff(SIMM_CS); } while (0)
#define Ports_SetOEOut(data) do { if (data) Ports_ControlOn(SIMM_OE); else Ports_ControlOff(SIMM_OE); } while (0)
#define Ports_SetWEOut(data) do { if (data) Ports_ControlOn(SIMM_WE); else Ports_ControlOff(SIMM_WE); } while (0)

void Ports_SetAddressDDR(uint32_t ddr);
void Ports_AddressDDR_RMW(uint32_t ddr, uint32_t modifyMask);
void Ports_SetDataDDR(uint32_t ddr);
void Ports_DataDDR_RMW(uint32_t ddr, uint32_t modifyMask);
void Ports_SetCSDDR(bool ddr);
void Ports_SetOEDDR(bool ddr);
void Ports_SetWEDDR(bool ddr);

void Ports_AddressPullups_RMW(uint32_t pullups, uint32_t modifyMask);
void Ports_DataPullups_RMW(uint32_t pullups, uint32_t modifyMask);
void Ports_SetCSPullup(bool pullup);
void Ports_SetOEPullup(bool pullup);
void Ports_SetWEPullup(bool pullup);

uint32_t Ports_ReadAddress(void);
uint32_t Ports_ReadData(void);
bool Ports_ReadCS(void);
bool Ports_ReadOE(void);
bool Ports_ReadWE(void);

// These two functions are for turning control signals on and off.
// Pass it a mask of SIMM_WE, SIMM_OE, and SIMM_CS.
#define Ports_ControlOn(mask)  do { PORTB |= (mask);  } while (0)
#define Ports_ControlOff(mask) do { PORTB &= ~(mask); } while (0)

#endif /* PORTS_H_ */
