#!/bin/bash

current_dir=$( pwd )

#cd ../part_c
#make
#sudo insmod blinkdrv.ko
#cd $current_dir

make
echo "" > /dev/usb/blinkstick0
sudo ./blink "~/linux-labs"
make clean
