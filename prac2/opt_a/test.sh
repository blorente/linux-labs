#!/bin/bash
gcc -g -Wall leds_user.c -o ledctl
./ledctl 0
rm ledctl