# Create a list of all source files specific to the PC
set(HWSOURCES
	hal/pc/board.c
	hal/pc/flash_4mbit.c
	hal/pc/gpio.c
	hal/pc/gpio_sim.c
	hal/pc/main.cpp
	hal/pc/mainwindow.cpp
	hal/pc/mainwindow.ui
	hal/pc/parallel_bus.c
	hal/pc/programmerthread.cpp
	hal/pc/spi.c
	hal/pc/usbcdc.c
	hal/pc/usbsimulationmanager.cpp
)

# Use Qt on the PC build to create a nice GUI
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)
