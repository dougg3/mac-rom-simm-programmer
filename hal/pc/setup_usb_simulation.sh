#!/bin/sh

F_ACM_MODULE_SYSFS_PATH=/sys/module/usb_f_acm
DUMMY_HCD_MODULE_SYSFS_PATH=/sys/module/dummy_hcd
SIMMPROGRAMMER_VID=0x16d0
SIMMPROGRAMMER_PID=0x06aa

find_configfs_dir() {
	# Now grab the current configfs location
	FOUND_DIR=$(mount | grep -m 1 "^configfs on" | sed "s/^configfs on \(.*\) type .*$/\1/")
	if [ ! -d "$FOUND_DIR" ]; then
		echo "Error: Something strange happened while trying to locate the configfs mountpoint. You should make sure you have configfs mounted in the usual location (/sys/kernel/config)." >&2
		exit 1
	fi

	echo $FOUND_DIR
}

start_gadget() {
	# If the acm module isn't loaded, try loading it manually
	if [ ! -d $F_ACM_MODULE_SYSFS_PATH ]; then
		modprobe usb_f_acm
		if [ $? -ne 0 ]; then
			echo "Error: Unable to load usb_f_acm kernel module." >&2
			exit 1
		fi
	fi

	# Same with dummy_hcd
	if [ ! -d $DUMMY_HCD_MODULE_SYSFS_PATH ]; then
		modprobe dummy_hcd
		if [ $? -ne 0 ]; then
			echo "Error: Unable to load dummy_hcd kernel module." >&2
			echo "Note: Many Linux distributions don't come with this module by default. You may need to compile it yourself. See the README file for more details." >&2
			exit 1
		fi
	fi

	# Make sure we have a dummy_udc to use
	if [ ! -d /sys/class/udc/dummy_udc.0 ]; then
		echo "Error: Can't find a dummy UDC device to use in /sys/class/udc" >&2
		exit 1
	fi

	# locate configfs, mount it if it's not already loaded
	CONFIGFS_LINE_COUNT=$(mount | grep -c "^configfs on ")
	if [ $CONFIGFS_LINE_COUNT -lt 1 ]; then
		# Try to mount it ourselves if we couldn't
		mount -t configfs configfs /sys/kernel/config
	fi

	# Check one more time
	CONFIGFS_LINE_COUNT=$(mount | grep -c "^configfs on ")
	if [ $CONFIGFS_LINE_COUNT -lt 1 ]; then
		echo "Error: Unable to find configfs. Is your kernel configured with configfs support?" >&2
		exit 1
	fi

	# Now grab the current configfs location
	CONFIGFS_DIR=$(find_configfs_dir)

	# Check if we have the usb_gadget subdir
	if [ ! -d "$CONFIGFS_DIR/usb_gadget" ]; then
		echo "Error: The configfs filesystem doesn't seem to contain a usb_gadget subdirectory. Make sure your kernel is compiled with configfs support for USB gadgets." >&2
		exit 1
	fi

	# Make sure we can't find an existing gadget that's set up as us
	cd "$CONFIGFS_DIR/usb_gadget"
	for g in */; do
		if [ -d "$g" ]; then
			VID=$(cat "$g/idVendor")
			PID=$(cat "$g/idProduct")
			if [ $VID = $SIMMPROGRAMMER_VID -a $PID = $SIMMPROGRAMMER_PID ]; then
				echo "Error: The gadget is already started" >&2
				exit 1
			fi
		fi
	done

	# Find the first empty gadget slot to use
	COUNTER=1
	while [ -d "$CONFIGFS_DIR/usb_gadget/g$COUNTER" ]; do
		COUNTER=$((COUNTER+1))
	done
	NEWGADGET="g$COUNTER"

	# Create the gadget and configure it
	mkdir "$CONFIGFS_DIR/usb_gadget/$NEWGADGET"
	cd "$CONFIGFS_DIR/usb_gadget/$NEWGADGET"
	echo $SIMMPROGRAMMER_VID > idVendor
	echo $SIMMPROGRAMMER_PID > idProduct
	mkdir strings/0x0409
	echo "Doug Brown" > strings/0x0409/manufacturer
	echo "Mac ROM SIMM Programmer" > strings/0x0409/product
	echo "0" > strings/0x0409/serialnumber
	mkdir functions/acm.GS0
	echo 0 > functions/acm.GS0/console
	mkdir configs/c.1
	ln -s functions/acm.GS0 configs/c.1/

	# Set up permissions so any user can connect/disconnect the USB device.
	# This will allow us to hotplug directly inside the PC build while not root.
	chmod 666 UDC

	# Don't plug it in yet; use connect_gadget for that
	echo "Created dummy SIMM programmer gadget"
}

stop_gadget() {
	FOUND_ANY=false

	# Now grab the current configfs location
	CONFIGFS_DIR=$(find_configfs_dir)

	# Check if we have the usb_gadget subdir
	if [ -d "$CONFIGFS_DIR/usb_gadget" ]; then
		# Find any existing gadget that is set up as us, and destroy it
		cd "$CONFIGFS_DIR/usb_gadget"
		for g in */; do
			if [ -d "$g" ]; then
				VID=$(cat "$g/idVendor")
				PID=$(cat "$g/idProduct")
				if [ $VID = $SIMMPROGRAMMER_VID -a $PID = $SIMMPROGRAMMER_PID ]; then
					# Disconnect the gadget if it's already connected to a UDC
					CURUDC=$(cat $g/UDC)
					if [ x$CURUDC != x ]; then
						echo "" > $g/UDC
					fi
					# Then destroy the rest of it
					rm $g/configs/c.1/acm.GS0
					rmdir $g/configs/c.1
					rmdir $g/functions/acm.GS0
					rmdir $g/strings/0x0409
					rmdir $g
					FOUND_ANY=true
				fi
			fi
		done
	fi

	if $FOUND_ANY; then
		echo "Deleted dummy SIMM programmer gadget"
	else
		echo "Error: Couldn't find a dummy SIMM programmer gadget to delete." >&2
		exit 1
	fi
}

# Make sure they supplied a command
if [ $# -ne 1 ]; then
	echo "Usage: $0 <start|stop>" >&2
	exit 1
fi

# Make sure we are root
if [ $(whoami) != "root" ]; then
	echo "This script has to be run as root." >&2
	exit 1
fi

# Parse the command and run the function for it
if [ $1 = "start" ]; then
	start_gadget
elif [ $1 = "stop" ]; then
	stop_gadget
else
	echo "Invalid command: $1" >&2
	exit 1
fi
