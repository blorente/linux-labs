#!/bin/bash
make clean
sudo rmmod modlist
sudo dmesg | tail