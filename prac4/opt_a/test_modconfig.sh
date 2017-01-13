#!/bin/bash
./mount.sh
cat /proc/modconfig
echo timer_period_ms 500 > /proc/modconfig
cat /proc/modconfig
./unmount.sh
dmesg | tail