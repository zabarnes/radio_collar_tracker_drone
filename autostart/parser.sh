#!/bin/bash
timestamp(){
	date
}

if [ ! -e /sys/class/gpio/gpio60 ]
	then
	echo 60 > /sys/class/gpio/export
	echo out > /sys/class/gpio/gpio60/direction
fi
run=true
threshold="35"
log="/home/debian/rctparser.log"
while $run; do
	numFiles=$(ls -l /media/RAW_DATA/rct/ | wc -l)
	sleep 30
	if [[ $(ls -l /media/RAW_DATA/rct/ | wc -l) -ne numFiles ]]
		then
		echo "0" > /sys/class/gpio/gpio60/direction
		run=false;
		sudo kill -9 `pgrep collarTracker`
		echo "$(timestamp): Killing collarTracker"
		echo "$(timestamp): Killing collarTracker" >> $log
	else
		echo "1" > /sys/class/gpio/gpio60/direction
		echo "$(timestamp): collarTracker is still alive"
		echo "$(timestamp): collarTracker is still alive" >> $log
	fi
done
echo "0" > /sys/class/gpio/gpio60/direction
