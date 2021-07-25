/*
 * main.cpp
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

#include "mainwindow.h"
#include "programmerthread.h"
#include "usbsimulationmanager.h"
extern "C"
{
#include "usbcdc_hw.h"
}
#include <QApplication>
#include <QDir>
#include <QMessageBox>

/**
 * @brief Performs a bunch of sanity checks to make sure we are empowered to simulate a USB device
 * @return True if we are good to go, false if an error occurred.
 *
 * If there's a problem, show a message informing the user what's wrong.
 */
bool DoSanityChecks(void)
{
	// Make sure configfs is available
	if (USBSimulationManager::configFSPath().isEmpty())
	{
		QMessageBox::critical(nullptr, "Mountpoint missing", "The configfs filesystem doesn't seem to be mounted on this system. Try running the following command to mount it:\n\n"
							  "sudo mount -t configfs configfs /sys/kernel/config\n\n");
		return false;
	}

	if (USBSimulationManager::localGadgetDir().isEmpty())
	{
		QMessageBox::critical(nullptr, "USB gadget not ready", "Could not locate the simulated USB gadget. Have you run the setup_usb_simulation.sh script as root?");
		return false;
	}

	if (USBSimulationManager::localGadgetPortDevice().isEmpty())
	{
		QMessageBox::critical(nullptr, "USB gadget not ready", "Could not locate the TTY device for the simulated USB gadget. Have you run the setup_usb_simulation.sh script as root?");
		return false;
	}

	// We made it, we should be good to go.
	return true;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	// Start by performing a bunch of sanity checks to make sure we actually have a simulated
	if (!DoSanityChecks())
	{
		return EXIT_FAILURE;
	}

	// Start out with the simulated device disconnected
	USBSimulationManager::disconnectGadget();

	// Tell the simulated device which port to open
	USBCDC_SetPortName(USBSimulationManager::localGadgetPortDevice().toUtf8().constData());

	// Run the programmer thread. Note: this is crude -- I just leave it as a dangling object
	// with no owner. This seems to be the best way to terminate the thread when closing the
	// program though. If I intentionally try to stop the thread, it doesn't work properly.
	ProgrammerThread *programmer = new ProgrammerThread();
	programmer->start();

	// Now we can initialize the system
	MainWindow w;
	w.show();

	return a.exec();
}
