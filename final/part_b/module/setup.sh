#!/bin/bash
make
sudo insmod fifomod.ko fifo_num=$1
dmesg | tail