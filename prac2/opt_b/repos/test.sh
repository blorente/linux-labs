#!/bin/bash

current_dir=$( pwd )

#cd ../part_c
#make
#sudo insmod blinkdrv.ko
#cd $current_dir

make
echo "" > /dev/usb/blinkstick0
#sudo ./blink 0 "$current_dir/test_repos/clean"
sudo ./blink 1 "$current_dir/test_repos/staging"
#sudo ./blink 2 "$current_dir/test_repos/unsaved"
make clean
