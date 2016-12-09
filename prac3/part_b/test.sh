#!/bin/bash
make
sudo insmod fifomod.ko
sudo rmmod fifomod
make clean
dmesg | tail