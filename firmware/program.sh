#!/bin/sh

echo 355 > /sys/class/gpio/export
echo 0 > /sys/class/gpio/gpio355/value
sleep 1
echo 1 > /sys/class/gpio/gpio355/value
sleep 1

stm32flash -w ../firmware/build/ch.hex /dev/ttyUSB0
python rts.py &
echo 0 > /sys/class/gpio/gpio355/value
sleep 1
echo 1 > /sys/class/gpio/gpio355/value

