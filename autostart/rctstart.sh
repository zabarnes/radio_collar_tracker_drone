#!/bin/sh
### BEGIN INIT INFO
# Provides:          rct start
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: rctstart
# Description:       radio collar start
### END INIT INFO

# setup here
timestamp(){
	date
}

log="/home/debian/rct.log"
# check for autostart file!
if [ ! -e /sys/class/gpio/gpio30 ]
	then
	echo 30 > /sys/class/gpio/export
fi
if [ ! -e /sys/class/gpio/gpio60 ]
	then
	echo 60 > /sys/class/gpio/export
fi
echo low > /sys/class/gpio/gpio60/direction
if [ ! -e /home/debian/autostart ]
	then
	exit
fi

stateVal="startWait"
echo "$(timestamp): Starting..." >> $log
echo "$(timestamp): Starting..."
while true
do
	case $stateVal in
		"startWait" )
		# State 1 - wait for start
			switchVal=$(cat /sys/class/gpio/gpio30/value)
			until [ "$switchVal" = "1" ]; do
				sleep 3
				switchVal=$(cat /sys/class/gpio/gpio30/value)
			done
			echo "$(timestamp): Received start signal!" >> $log
			echo "$(timestamp): Received start signal!"
			stateVal="startgo"
			;;
		"startgo" )
		# State 2 - go for start, initialize
			cpufreq-set -g performance >> $log 2>&1
			cpufreq-info >> $log 2>&1
			/home/debian/xcode/collarTracker &
			sudo /home/debian/parser.sh >> $log &
			echo "$(timestamp): Started program!" >> $log
			echo "$(timestamp): Started program!"
			stateVal="endWait"
			;;
		"endWait" )
			switchVal=$(cat /sys/class/gpio/gpio30/value)
			until [ "$switchVal" = "0" ]; do
				sleep 3
				switchVal=$(cat /sys/class/gpio/gpio30/value)
			done
			echo "$(timestamp): Received stop signal!" >> $log
			echo "$(timestamp): Received stop signal!"
			stateVal="endgo"
			;;
		"endgo" )
			sudo kill -9 `pgrep collarTracker`
			echo "$(timestamp): Ended program!" >> $log
			echo "$(timestamp): Ended program!"
			stateVal="startWait"
			;;
	esac
done
