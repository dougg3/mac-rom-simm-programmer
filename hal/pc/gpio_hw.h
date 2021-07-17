/*
 * gpio_hw.h
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

#ifndef HAL_PC_GPIO_HW_H_
#define HAL_PC_GPIO_HW_H_

/// Enum representing the different GPIO ports available on the PC simulator.
/// Used with the GPIOPin struct.
enum {
	/// The PC simulated hardware is laid out in a special way so that pins that
	/// belong together are grouped onto virtual "ports". All of the address pins
	/// comprise one port, all of the data pins comprise another port, and other
	/// miscellaneous pins like LEDs and parallel bus control pins comprise one
	/// additional port.
	GPIOMISC,
	GPIOADDR,
	GPIODATA,
};

#endif /* HAL_PC_GPIO_HW_H_ */
