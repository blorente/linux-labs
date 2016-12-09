#!/bin/bash
make
sudo insmod modlist.ko
sudo echo "add 21" > /proc/modlist
sudo echo "add 4" > /proc/modlist
sudo echo "add 5" > /proc/modlist
sudo echo $( cat /proc/modlist )
sudo echo "add 3" > /proc/modlist
sudo echo $( cat /proc/modlist )
sudo echo "remove 4" > /proc/modlist
sudo echo $( cat /proc/modlist )
sudo echo "cleanupsd" > /proc/modlist
sudo echo $( cat /proc/modlist )
sudo echo "cleanup" > /proc/modlist
sudo echo $( cat /proc/modlist )
make clean
sudo rmmod modlist
sudo dmesg | tail
