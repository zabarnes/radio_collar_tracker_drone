#!/bin/bash
sudo cp rctstart.sh /etc/init.d/
cp parser.sh ~
sudo update-rc.d rctstart.sh defaults
sudo reboot
