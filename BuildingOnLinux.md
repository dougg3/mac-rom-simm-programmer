# Introduction #

I haven't yet figured out a preferable way to package binaries for Linux, so for now, I'm just going to provide compile instructions.


# Details #

These instructions should work with Ubuntu 9.10 and later. I'm unsure of how it works with other distributions. You can also make it work with Ubuntu 9.04 if you install a newer version of libudev and its development package.

First of all, make sure you have Qt 4.x and its related stuff installed. Most distributions provide Qt, so make sure you have your distribution's Qt development packages installed. On Debian-based systems, the development package is called `libqt4-dev` (installing this package should give you everything you need). Alternatively, you can [download the Qt SDK](http://qt.nokia.com/products/qt-sdk) to get what you need. If for some reason the version of Qt supplied by your distribution doesn't work, the Qt SDK will work fine.

You will also need libudev and its development package. On Debian-based systems, this development package (which will also install libudev due to its dependencies) is called `libudev-dev`.

Once that is taken care of, you need to create a folder where you'll compile this stuff:

```
mkdir SIMMProgrammer
cd SIMMProgrammer
```

Get the source code you need:

```
git clone https://github.com/dougg3/mac-rom-simm-programmer.software.git
git clone https://github.com/dougg3/doug-qextserialport-linuxnotifications.git
```

Compile it:

```
mkdir build
cd build
qmake ../mac-rom-simm-programmer.software/
make
```

The code doesn't currently compile with Qt 5, so if you have a newer system, you may need to substitute the command "qmake-qt4" in place of "qmake" to ensure it uses Qt 4.

You should have an executable called SIMMProgrammer in the build directory now. You can run it in the command line or create a shortcut for it or whatever.

```
./SIMMProgrammer
```

It may not talk to the SIMM programmer correctly because of udev rules. Create a file called `99-simm-programmer.rules` in `/etc/udev/rules.d`, containing:

```
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="06aa", MODE="0666", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
```

Then, reload udev's rules by typing:

```
sudo udevadm control --reload-rules
```

Now, unplug and replug the programmer board and you should be in business!