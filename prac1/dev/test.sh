#!/bin/bash
make
sudo insmod modlist.ko
echo "Hello 21" > /proc/modlist
cat /proc/modlist
echo "Hello 3" > /proc/modlist
echo "Hello 1" > /proc/modlist
cat /proc/modlist
make clean
sudo rmmod modlist
sudo dmesg | grep Modlist
