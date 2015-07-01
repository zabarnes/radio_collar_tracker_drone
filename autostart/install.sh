#!/bin/bash
sudo cp rctstart.sh /etc/init.d/
sudo update-rc.d rctstart.sh defaults
sudo reboot
