#!/bin/bash
make
sudo insmod blinkdrv.ko
echo 1:0x001100,3:0x000007,5:0x090000 > /dev/usb/blinkstick0
sleep 1
echo 1:0x001100,3:0x000007,5:0x090000 > /dev/usb/blinkstick0
sleep 1
echo 1:0x001100,3:0x000007,5:0x090000 > /dev/usb/blinkstick0
sleep 1
echo 1:0x001100,3:0x000007,5:0x090000 > /dev/usb/blinkstick0
sleep 1
sudo rmmod blinkdrv
make clean