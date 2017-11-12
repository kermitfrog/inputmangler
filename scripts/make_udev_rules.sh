#!/bin/bash
if [[ $1 == "" ]] ; then
	echo "which user group should have rights to access the input devices? [inputmangler]"
	read GRP
	if [[ "$GRP" == "" ]] ; then
		GRP="inputmangler"
	fi
else
	GRP=$1
fi

FILE="80-inputmangler.rules"

sed --expression "s/Vendor=0000/Vendor=x/" /proc/bus/input/devices | grep --extended-regexp "^I:.*Vendor=[0-9]" --after-context=1 | awk '/^I:.*/{x=$0;next}/^N:.*/{print $0"\n"x;}/^--.*/{print}' - > $FILE
sed --in-place --expression "s/--//" $FILE
sed --in-place --expression "s/^N: Name=/#/" $FILE
sed --in-place --expression "s/^I: Bus=[0-9]* Vendor=/#ATTRS\{idVendor\}==\"/" $FILE
sed --in-place --expression "s/ Product=/\", ATTRS\{idProduct\}==\"/" $FILE
sed --in-place --expression "s/ Version=.*/\", MODE=\"0660\", GROUP=\"$GRP\" RUN+=\"\/usr\/bin\/killall -HUP inputmangler\"/" $FILE
chmod 644 $FILE

echo ""
echo "Delete the comment (#) sign of the appropriate lines (starting with ATTRS{idVendor}) in $FILE to enable the Devices you want to use with inputmangler. Then execute the following line to install the file in /etc/udev/rules.d/"
echo "sudo cp $FILE /etc/udev/rules.d/ ; sudo chmod 644 /etc/udev/rules.d/80-inputmangler.rules"
echo "" 
echo "Please note, that udev does not set the permissions for PS/2 devices. See the README, section ##INS## on how to do that."
echo ""
echo "After that: to activate it, reboot or set the permissions of your input devices manually and run"
echo "sudo modprobe inputmangler ; chown :$GRP \/dev\/virtual_kbd \/dev\/virtual_mouse ; chmod 660 \/dev\/virtual_kbd \/dev\/virtual_mouse"

