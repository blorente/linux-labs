#!/bin/bash
sudo rmmod fifomod
make clean
dmesg | tail