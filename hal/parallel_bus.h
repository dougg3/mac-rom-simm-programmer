/*
 * parallel_bus.h
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef HAL_PARALLEL_BUS_H_
#define HAL_PARALLEL_BUS_H_

#include <stdint.h>
#include <stdbool.h>

void ParallelBus_Init(void);

void ParallelBus_SetAddress(uint32_t address);
void ParallelBus_SetData(uint32_t data);
void ParallelBus_SetCS(bool high);
void ParallelBus_SetOE(bool high);
void ParallelBus_SetWE(bool high);

void ParallelBus_SetAddressDir(uint32_t outputs);
void ParallelBus_SetDataDir(uint32_t outputs);
void ParallelBus_SetCSDir(bool output);
void ParallelBus_SetOEDir(bool output);
void ParallelBus_SetWEDir(bool output);

void ParallelBus_SetAddressPullups(uint32_t pullups);
void ParallelBus_SetDataPullups(uint32_t pullups);
void ParallelBus_SetCSPullup(bool pullup);
void ParallelBus_SetOEPullup(bool pullup);
void ParallelBus_SetWEPullup(bool pullup);

uint32_t ParallelBus_ReadAddress(void);
uint32_t ParallelBus_ReadData(void);
bool ParallelBus_ReadCS(void);
bool ParallelBus_ReadOE(void);
bool ParallelBus_ReadWE(void);

void ParallelBus_WriteCycle(uint32_t address, uint32_t data);
uint32_t ParallelBus_ReadCycle(uint32_t address);
void ParallelBus_Read(uint32_t startAddress, uint32_t *buf, uint16_t len);

#endif /* HAL_PARALLEL_BUS_H_ */
