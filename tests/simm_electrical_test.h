/*
 * simm_electrical_test.h
 *
 *  Created on: Nov 26, 2011
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

#ifndef SIMM_ELECTRICAL_TEST_H_
#define SIMM_ELECTRICAL_TEST_H_

#include <stdint.h>

int SIMMElectricalTest_Run(void (*errorHandler)(uint8_t, uint8_t));

#endif /* SIMM_ELECTRICAL_TEST_H_ */
