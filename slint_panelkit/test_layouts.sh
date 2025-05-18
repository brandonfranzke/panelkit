#!/bin/bash
# Test various layout configurations

echo "Testing Portrait (480x640)"
make run WIDTH=480 HEIGHT=640 &
PID1=$!
sleep 5
kill $PID1

echo "Testing Landscape (640x480)"
make run WIDTH=640 HEIGHT=480 &
PID2=$!
sleep 5
kill $PID2

echo "Testing Square (600x600)"
make run WIDTH=600 HEIGHT=600 &
PID3=$!
sleep 5
kill $PID3

echo "Testing Small Portrait (320x480)"
make run WIDTH=320 HEIGHT=480 &
PID4=$!
sleep 5
kill $PID4