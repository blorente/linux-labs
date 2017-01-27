#!/bin/bash
make
sudo insmod fifomod.ko
dmesg | tail