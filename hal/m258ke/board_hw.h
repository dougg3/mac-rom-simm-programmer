/*
 * board_hw.h
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

#ifndef HAL_M258KE_BOARD_HW_H_
#define HAL_M258KE_BOARD_HW_H_

#include "gpio_hw.h"
#include "../gpio.h"
#include "hardware.h"
#include "../usbcdc.h"

#define BOARD_LED_INVERTED true
#define BOARD_SUPPORTS_PULLDOWNS true

/** Gets the GPIO pin on the board that controls the status LED
 *
 * @return The status LED pin
 */
static inline GPIOPin Board_LEDPin(void)
{
	return GPIO_PIN(GPIOB, 14);
}

/** Jumps to the bootloader
 *
 */
static inline void Board_EnterBootloader(void)
{
	// Insert a small delay to ensure that it arrives before rebooting.
	DelayMS(1000);

	// Disable interrupts so nothing weird happens...
	DisableInterrupts();

	// Done with the USB interface -- the bootloader will re-initialize it.
	USBCDC_Disable();

	// Wait a little bit to let everything settle and let the program
	// close the port after the USB disconnect
	DelayMS(2000);

	// Clear reset status bits so that bootloader knows reset reason
	SYS->RSTSTS = (SYS_RSTSTS_PORF_Msk | SYS_RSTSTS_PINRF_Msk);

	// Boot to LDROM next time
	FMC->ISPCTL |= FMC_ISPCTL_BS_Msk;

	// Save a special flag in RAM to indicate we want to stay in the bootloader
	*(uint32_t *)(0x20003FFC) = 0xBADF00D5;

	// Reset!
	NVIC_SystemReset();

	// Should never get here, but just in case...
	while (1);
}

#endif /* HAL_M258KE_BOARD_HW_H_ */
