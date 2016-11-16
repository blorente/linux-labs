#!/bin/bash
gcc -g -Wall leds_user.c -o ledctl

for i in $(seq 1 $1)
do
	./ledctl 0x1
	echo "(Scroll)"
	sleep 0.33
	./ledctl 0x2
	echo "(Caps)"
	sleep 0.33
	./ledctl 0x4
	echo "(Num)"
	sleep 0.33
done

for i in $(seq 1 $2)
do
	./ledctl 0x7
	echo "(All)"
	sleep 0.1
	./ledctl 0x0
	echo "(None)"
	sleep 0.1
done


rm ledctl