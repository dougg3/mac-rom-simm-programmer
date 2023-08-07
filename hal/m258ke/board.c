/*
 * board.c
 *
 *  Created on: Jun 19, 2023
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

#include "board_hw.h"

/** Initializes any board hardware-specific stuff
 *
 */
void Board_Init(void)
{
	// Unlock protected registers so we can configure clocks, flash, WDT, etc.
	do
	{
		SYS->REGLCTL = 0x59UL;
		SYS->REGLCTL = 0x16UL;
		SYS->REGLCTL = 0x88UL;
	} while (SYS->REGLCTL == 0UL);

	// Enable 48 MHz internal high-speed RC oscillator
	CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

	// Wait until it's ready
	while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

	// Clock HCLK and USB from 48 MHz HIRC
	CLK->CLKSEL0 = (CLK->CLKSEL0 & (~(CLK_CLKSEL0_HCLKSEL_Msk | CLK_CLKSEL0_USBDSEL_Msk))) |
			(7 << CLK_CLKSEL0_HCLKSEL_Pos) | (0 << CLK_CLKSEL0_USBDSEL_Pos);

	// SystemCoreClock, CyclesPerUs, CyclesPerUs default to correct values already

	// Enable USB device controller
	CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk;

	// Enable timer 0
	CLK->APBCLK0 |= CLK_APBCLK0_TMR0CKEN_Msk;

	// Timer 0 clock source = 48 MHz HIRC
	CLK->CLKSEL1 = (CLK->CLKSEL1 & (~(CLK_CLKSEL1_TMR0SEL_Msk))) | (7UL << CLK_CLKSEL1_TMR0SEL_Pos);

	// Enable all GPIO
	CLK->AHBCLK |=
			CLK_AHBCLK_GPACKEN_Msk |
			CLK_AHBCLK_GPBCKEN_Msk |
			CLK_AHBCLK_GPCCKEN_Msk |
			CLK_AHBCLK_GPDCKEN_Msk |
			CLK_AHBCLK_GPECKEN_Msk |
			CLK_AHBCLK_GPFCKEN_Msk;

	// Start the timer, prescaler = 48, so 1 MHz
	TIMER0->CTL = TIMER_CTL_CNTEN_Msk | (3UL << TIMER_CTL_OPMODE_Pos) | 47;
}

/** Determines if a brownout was detected at startup
 *
 * @return True if a brownout was detected
 */
bool Board_BrownoutDetected(void)
{
	return false;
}
