#!/bin/bash
while true
do
	echo "Reading from /proc/modlist..."
	echo $( cat /proc/modlist )
	sleep 0.3s
	clear
done 