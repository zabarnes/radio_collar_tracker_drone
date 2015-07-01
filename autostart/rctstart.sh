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
			stateVal="startgo"
			;;
		"startgo" )
		# State 2 - go for start, initialize
			cpufreq-set -g performance >> $log 2>&1
			cpufreq-info >> $log 2>&1
			/home/debian/xcode/collarTracker > /home/debian/out.tmp &
			#/home/debian/ct > /home/debian/out.tmp &
			/home/debian/parser.sh < /home/debian/out.tmp >> $log 2>&1 &
			echo "$(timestamp): Started program!" >> $log
			stateVal="endWait"
			;;
		"endWait" )
			switchVal=$(cat /sys/class/gpio/gpio30/value)
			until [ "$switchVal" = "0" ]; do
				sleep 3
				switchVal=$(cat /sys/class/gpio/gpio30/value)
			done
			echo "$(timestamp): Received stop signal!" >> $log
			stateVal="endgo"
			;;
		"endgo" )
			kill -9 `pgrep collarTracker`
			#killall ct -s INT
			echo "$(timestamp): Ended program!" >> $log
			stateVal="startWait"
			;;
	esac
done
