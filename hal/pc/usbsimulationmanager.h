/*
 * usbsimulationmanager.h
 *
 *  Created on: Jul 25, 2021
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

#ifndef USBSIMULATIONMANAGER_H
#define USBSIMULATIONMANAGER_H

#include <QString>

class USBSimulationManager
{
public:
	static QString configFSPath();
	static QString localGadgetDir();
	static QString localGadgetPortDevice();
	static bool connectGadget();
	static bool disconnectGadget();
};

#endif // USBSIMULATIONMANAGER_H
