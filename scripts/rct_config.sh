led_num=17
switch_num=4
mav_port="/dev/ttyAMA0"
output_dir="/media/RAW_DATA/rct"
log_dir="/home/pi"

userconfig="$output_dir/config"

if [[ -e $userconfig ]]; then
	source $userconfig
fi
