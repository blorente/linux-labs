#!/bin/bash
echo "Writing to /proc/modlist..."
while true
do
	rand=$(( ( RANDOM % 10 )  + 1 ))
	echo "Inserting $rand"
	sudo echo "add $rand" > /proc/modlist
	sleep 0.5s
done