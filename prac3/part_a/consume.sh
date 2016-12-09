#!/bin/bash
while true
do
	echo "Consuming list..."
	sudo echo $( cat /proc/modlist )
	sudo echo "cleanup" > /proc/modlist
	sleep 2s
done