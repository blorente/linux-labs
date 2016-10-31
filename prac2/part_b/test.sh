#!/bin/bash
make
sudo insmod ledctl.ko
sudo echo "0x4" > /proc/ledctl
echo "(Num)"
sleep 3s
sudo echo "0x7" > /proc/ledctl
echo "(All)"
sleep 3s
sudo echo "0x3" > /proc/ledctl
echo "(Caps, Scroll)"
sleep 3s
sudo echo "0x0" > /proc/ledctl
echo "(None)"
sleep 3s
sudo echo "0x2" > /proc/ledctl
echo "(Caps)"
sleep 3s
sudo rmmod ledctl
make clean