#!/bin/bash
./setup.sh

cd FifoTest
make
cd ..

echo "Starting receiver..."
sudo ./FifoTest/fifotest -f "/proc/fifoproc" -r &
echo "Starting server..."
sudo ./FifoTest/fifotest -f /proc/fifoproc -s

./cleanup.sh