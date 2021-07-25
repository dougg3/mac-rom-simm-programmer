#include "usbsimulationmanager.h"
#include <QDir>

/// File in proc to find current mountpoints
#define MOUNTS_FILE					"/proc/mounts"

/// File type to look for in mounts for configfs
#define CONFIGFS_FS_TYPE			"configfs"

/// Subdirectory in configfs for usb gadget setup
#define CONFIGFS_USB_GADGET_SUBDIR	"usb_gadget"

/// The vendor ID of the SIMM programmer, in string format matching configfs idVendor
#define SIMMPROGRAMMER_VID_STRING	"0x16d0"

/// The product ID of the SIMM programmer, in string format matching configfs idProduct
#define SIMMPROGRAMMER_PID_STRING	"0x06aa"

/**
 * @brief Determines the path to configfs (if it's mounted)
 * @return The path to it, or an empty string if it's not mounted
 */
QString USBSimulationManager::configFSPath()
{
	// Open up /proc/mounts
	QFile mounts(MOUNTS_FILE);
	if (!mounts.open(QFile::ReadOnly))
	{
		return QString();
	}

	// Search for configfs as a mountpoint
	QByteArray line("l");
	while (!line.isEmpty())
	{
		line = mounts.readLine();
		QList<QByteArray> components = line.split(' ');
		if (components.count() >= 2 && components[0] == CONFIGFS_FS_TYPE)
		{
			// We found it! The second component is the mountpoint.
			return QString::fromUtf8(components[1]);
		}
	}

	// No need to close the file, it will automatically be closed when we return.
	// Return an empty string if we didn't find it
	return QString();
}

/**
 * @brief Gets the path to the gadget directory in configfs for the SIMM programmer, if it exists
 * @return The path to the gadget directory, or an empty string if we can't locate it
 */
QString USBSimulationManager::localGadgetDir()
{
	QString configFSDirPath = configFSPath();
	if (!configFSDirPath.isEmpty())
	{
		QDir dir(configFSDirPath);
		if (dir.exists() && dir.cd(CONFIGFS_USB_GADGET_SUBDIR))
		{
			foreach (QString const &g, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
			{
				QByteArray vendor, product;

				QDir gDir = dir;
				gDir.cd(g);

				QFile f(gDir.absoluteFilePath("idVendor"));
				if (f.open(QFile::ReadOnly))
				{
					vendor = f.readAll().trimmed();
					f.close();
				}
				f.setFileName(gDir.absoluteFilePath("idProduct"));
				if (f.open(QFile::ReadOnly))
				{
					product = f.readAll().trimmed();
					f.close();
				}

				// If it matches product and vendor ID, we can assume we got it.
				if (vendor == SIMMPROGRAMMER_VID_STRING &&
					product == SIMMPROGRAMMER_PID_STRING)
				{
					return gDir.absolutePath();
				}
			}
		}
	}
	// If we can't find it, bail
	return QString();
}
/*
 * usbsimulationmanager.cpp
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

/**
 * @brief Gets the path to the TTY device representing the simulated port
 * @return The path, or an empty string if we can't figure it out
 */
QString USBSimulationManager::localGadgetPortDevice()
{
	QString g = localGadgetDir();
	if (!g.isEmpty())
	{
		QDir gDir(g);
		if (gDir.cd("functions/acm.GS0"))
		{
			QFile f(gDir.absoluteFilePath("port_num"));
			if (f.open(QFile::ReadOnly))
			{
				QByteArray number = f.readAll().trimmed();
				f.close();
				QString path = "/dev/ttyGS" + QString::fromUtf8(number);
				if (QFile::exists(path))
				{
					return path;
				}
			}
		}
	}
	// If we can't find it, bail
	return QString();
}

/**
 * @brief Attempts to connect the simulated USB device to the host
 * @return True on success, false on failure
 */
bool USBSimulationManager::connectGadget()
{
	bool success = false;
	QDir gd(localGadgetDir());
	QFile controlFile(gd.absoluteFilePath("UDC"));
	if (controlFile.exists() && controlFile.open(QFile::WriteOnly))
	{
		success = controlFile.write("dummy_udc.0\n") > 0;
		controlFile.close();
	}
	return success;
}

/**
 * @brief Attempts to connect the simulated USB device to the host
 * @return True on success, false on failure
 */
bool USBSimulationManager::disconnectGadget()
{
	bool success = false;
	QDir gd(localGadgetDir());
	QFile controlFile(gd.absoluteFilePath("UDC"));
	if (controlFile.exists() && controlFile.open(QFile::WriteOnly))
	{
		success = controlFile.write("\n") > 0;
		controlFile.close();
	}
	return success;
}
