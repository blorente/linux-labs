#!/bin/bash
make
sudo insmod blinkdrv.ko
echo 1:0x001100,3:0x000007,5:0x090000 > /dev/usb/blinkstick0
echo "(1:G, 3:B, 5:R)"
sleep 1
echo 2:0x001100,4:0x000007,6:0x090000 > /dev/usb/blinkstick0
echo "(2:G, 4:B, 6:R)"
sleep 1
echo 1:0x00100A,3:0x070007 > /dev/usb/blinkstick0
echo "(1:Y, 3:V)"
sleep 1
echo 0:0x100000,2:0x010000,3:0x001000,4:0x000100,5:0x000010,6:0x000001 > /dev/usb/blinkstick0
echo "(1-6:order)"
sleep 1
sudo rmmod blinkdrv
make clean