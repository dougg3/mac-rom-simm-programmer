/*
 * board.c
 *
 *  Created on: Jul 17, 2021
 *      Author: Doug
 *
 * Copyright (C) 2011-2021 Doug Brown
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

#include "board_hw.h"

/** Initializes any board hardware-specific stuff
 *
 */
void Board_Init(void)
{
	// Nothing to do on PC
}

/** Determines if a brownout was detected at startup
 *
 * @return True if a brownout was detected
 */
bool Board_BrownoutDetected(void)
{
	// There is no such thing on the PC simulator
	return false;
}
