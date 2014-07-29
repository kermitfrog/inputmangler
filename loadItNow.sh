#!/bin/bash
service udev restart
insmod inputdummy/inputdummy.ko
chown :$1 /dev/virtual_kbd /dev/virtual_mouse
chmod 660 /dev/virtual_kbd /dev/virtual_mouse

