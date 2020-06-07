Compiling
=========

This is how to compile the firmware on linux:

1. Check out the ChibiOS submodule by running `git submodule update --init`. Alternatively, modify the Makefile to point to your existing ChibiOS root.
2. Run the `make`command.
3. Firmware is available in the `build` subdirectory.


Programming
===========

The STM32 microcontroller can be flashed from USB using the 'stm32flash-code' tool. The microcontroller needs to boot into the bootloader for this to work. Follow these steps in order to achieve this:

1. Activate the GPIO capabilities of the CP2105 USB bridge: `echo 355 > /sys/class/gpio/export`. The Number (355) may vary on your system.
2. Reset the microcontroller. The BOOT0 pin is high by default, starting the bootloader in reset:
```
echo 0 > /sys/class/gpio/gpio355/value
sleep 1
echo 1 > /sys/class/gpio/gpio355/value
sleep 1
```
3. Flash the firmware: `stm32flash -w build/ch.hex /dev/ttyUSB0`
4. Pull the BOOT0 pin low. The pin is connected to the RTS. Helper program rts.py keeps the pin low for 10 seconds. Thus, running `python rts.py &` fulfils this step. The subsequent reset in the next step must take place within 10 secods.
5. Reset the microontroller once again:
```
echo 0 > /sys/class/gpio/gpio355/value
sleep 1
echo 1 > /sys/class/gpio/gpio355/value
```

A helper program `program.sh` automates the above steps.
