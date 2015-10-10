# Introduction #

I had a lot of trouble getting my `udev` rule correct for the SIMM programmer board, so I wanted to explain more about how I figured out what to do.


# Details #

We need a `udev` rule on Linux because it defaults to giving a USB CDC modem port permissions such that only `root` and members of the `dialout` group can talk to it. The rule simply gives read and write permissions to all users, and gives it to the plugdev group rather than the dialout group.

`udev` rules work by matching certain attributes and performing actions in response if the attributes match. In this case, we want to match on the USB vendor and device ID.

To get a list of attributes for a specific device (assuming `/dev/ttyACM0` in this example), type:

```
sudo udevadm info --name=/dev/ttyACM0 --attribute-walk
```

This will give an awesome list of all of that device's attributes, along with a quick explanation of how the rules will work.

Rule for the SIMM programmer board:

```
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="06aa", MODE="0666", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
```

I added the `ACTION=="add"` portion because I really only care about when the device is first being added to the system -- that's a perfect time to change the permissions. The rest of the rule matches a USB device with the provided vendor and product IDs. In response, it will change the mode and group of the device node.

Many newer Linux distributions use `ModemManager` to automatically detect modems when USB serial devices are plugged in by opening the port and doing things with it briefly. This behavior interferes with the SIMM programmer software, so setting the `ID\_MM\_DEVICE\_IGNORE` environment variable forces `ModemManager` to skip auto-detection on the port.