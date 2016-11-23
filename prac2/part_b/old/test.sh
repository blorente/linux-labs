#!/bin/bash
gcc -g -Wall ledctl_invoke.c -o ledctl
./ledctl 0x4
echo "(Num)"
sleep 3s
./ledctl 0x7
echo "(All)"
sleep 3s
./ledctl 0x3
echo "(Caps, Scroll)"
sleep 3s
./ledctl 0x0
echo "(None)"
sleep 3s
./ledctl 0x2
echo "(Caps)"
sleep 3s
rm ledctl