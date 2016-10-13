#!/bin/bash
echo "Hello 21" > /proc/modlist
cat /proc/modlist
echo "Hello 3" > /proc/modlist
echo "Hello 1" > /proc/modlist
cat /proc/modlist
sudo rmmod modlist
sudo dmesg | grep Modlist
