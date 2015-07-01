#!/bin/bash
if [ ! -e /sys/class/gpio/gpio60 ]
	then
	echo 60 > /sys/class/gpio/export
fi
run=true
threshold="5"
while $run; do
	sleep 3
	modify=$(date -r out.tmp +%s)
	current=$(date +%s)
	diff=`expr $current - $modify`

	if [ "$diff" -gt "$threshold" ]
		then
		echo low > /sys/class/gpio/gpio60/direction
		run=false;
		sudo kill -9 `pgrep collarTracker`
	else
		echo high > /sys/class/gpio/gpio60/direction
	fi
done
echo low > /sys/class/gpio/gpio60/direction
