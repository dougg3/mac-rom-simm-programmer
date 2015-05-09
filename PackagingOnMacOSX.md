# Introduction #

To start, you need the Qt SDK installed, git for Mac OS X, and mercurial for Mac OS X. This is tested on Mac OS X 10.8 Mountain Lion, but probably works on older versions too.

# Details #

The versions of Qt that are provided are not compiled with i386 support enabled, so compile it yourself with i386 and x86\_64 support. I just do this for packaging purposes so older machines can still run the program. Theoretically it should still function on OS X 10.5, although I don't have the ability to test it.

## Download the Qt 4.8.3 source code: ##

http://releases.qt-project.org/qt4/source/qt-everywhere-opensource-src-4.8.3.tar.gz

## Compile and install Qt: ##

`./configure -opensource -release -no-sql-mysql -nomake examples -nomake demos -no-dbus -no-phonon -no-webkit -no-qt3support -no-xmlpatterns -no-multimedia -no-audio-backend -no-phonon-backend -no-svg -no-script -no-scripttools -no-declarative -no-declarative-debug -arch x86 -arch x86_64`

Or, if you are doing a 10.4-compatible Universal Binary:

`./configure -opensource -release -no-sql-mysql -nomake examples -nomake demos -no-dbus -no-phonon -no-webkit -no-qt3support -no-xmlpatterns -no-multimedia -no-audio-backend -no-phonon-backend -no-svg -no-script -no-scripttools -no-declarative -no-declarative-debug -carbon -universal -sdk /Developer/SDKs/MacOSX10.4u.sdk`

`make`

`sudo make install`

## Grab the Mac ROM SIMM control program: ##

`git clone https://code.google.com/p/mac-rom-simm-programmer.software/`

If you're making a universal binary for 10.4, edit ROMSIMMFlasher.pro inside the checked-out directory, remove x86\_64 from macx:CONFIG, and put ppc in its place.

## Grab my custom fork of QextSerialPort: ##

`hg clone https://code.google.com/r/doug-qextserialport-linuxnotifications/`

## Set up Qt Creator ##

  * Open Qt Creator (we're going to add the newly-compiled Qt to the list of Qt versions)
  * Go to Preferences->Build & Run->Qt Versions->Add
  * Because the OS X file chooser will not let you get into /usr/local, you need to cheat to get it to show it...
    * Switch to the Finder for a moment, go to Go->Go to Folder... and open up /usr/local
    * Drag the Trolltech folder to the file selection window in Qt Creator
    * Now navigate to your installed qmake executable (/usr/local/Trolltech/Qt-4.8.3/bin/qmake) and pick it

## Build the project ##

  * Open the project (mac-rom-simm-programmer.software/ROMSIMMFlasher.pro) in Qt Creator
  * It will ask you to create build configurations. Choose Create Build Configuration: manually
  * Only check the Release version of your custom Qt 4.8.3 install
  * Go to Build->Build All to build the executable

## Deploy the program ##

Now the program is built, so convert it to a deployable standalone application...

`/usr/local/Trolltech/Qt-4.8.3/bin/macdeployqt ../ROMSIMMFlasher-build-desktop-Qt_4_8_3__Qt-4_8_3__Release/SIMMProgrammer.app/`

## All done ##

Now it's ready for users!

Note: If I ever have to use more of the Qt libraries (such as WebKit), patches available on [this bug report](https://bugreports.qt-project.org/browse/QTBUG-23258) may be necessary.