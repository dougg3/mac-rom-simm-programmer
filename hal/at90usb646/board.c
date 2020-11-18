/*
 * board.c
 *
 *  Created on: Nov 15, 2020
 *      Author: Doug
 */

#include "board_hw.h"
#include <avr/io.h>

/// Whether we detected that the board had a brownout event
static bool brownout = false;

/** Initializes any board hardware-specific stuff
 *
 */
void Board_Init(void)
{
	// Figure out if a brownout occurred
	if (MCUSR & (1 << BORF))
	{
		MCUSR = 0;
		brownout = true;
	}
}

/** Determines if a brownout was detected at startup
 *
 * @return True if a brownout was detected
 */
bool Board_BrownoutDetected(void)
{
	return brownout;
}
