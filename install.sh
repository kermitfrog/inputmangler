#!/bin/bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

echo "inputmangler can run from any directory, so it does not need to be installed system wide - but that doesn't mean you can't"
echo "install to /usr/local/?  [y|N]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' ]]; then
	make install
fi

echo "inputdummy however is a kernel module and should be installed system wide. Do it? [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == "" ]]; then
	cd ../inputdummy
	make
	sudo make install
	cd ..
fi

echo "Append inputdummy to the list of modules that are loaded on boot?  [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == "" ]]; then
	sudo echo inputdummy >> /etc/modules
fi

echo ""
echo "which user group should have rights to access the input devices? [$USER]"
read GRP
if [[ "$GRP" == "" ]] ; then
	GRP=$USER
fi



echo "Append lines to rc.local, to set the correct rights to the inputdummy devices? [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == "" ]]; then
	sudo echo "chown :$GRP /dev/virtual_kbd /dev/virtual_mouse" >> /etc/modules
	sudo echo "chmod 660 /dev/virtual_kbd /dev/virtual_mouse" >> /etc/modules
fi

echo "Done!"
echo "if your kernel is updated, you will need to reinstall inputdummy. Do this by going into the inputdummy directory and executing"
echo "make && make install"
echo ""

sed -e "s/Vendor=0000/Vendor=x/" /proc/bus/input/devices | grep -E "^I:.*Vendor=[0-9]" -A 1 > 99-input.rules
sed -ie "s/--//" 99-input.rules
sed -ie "s/^N: Name=/#/" 99-input.rules
sed -ie "s/^I: Bus=[0-9]* Vendor=/#ATTRS\{idVendor\}==\"/" 99-input.rules
sed -ie "s/ Product=/\", ATTRS\{idProduct\}==\"/" 99-input.rules
sed -ie "s/ Version=.*/\", MODE=\"0660\", GROUP=\"$GRP\"/" 99-input.rules

echo ""
echo "Delete the comment (#) sign of the appropriate lines (starting with ATTRS{idVendor}) in 99-input.rules to enable the Devices you want to use with inputmangler. Then execute the following line to install the file in /etc/udev/rules.d/"
echo "sudo cp 99-input.rules /etc/udev/rules.d/ ; sudo chmod 644 /etc/udev/rules.d/99-input.rules"
echo "" 
echo "After that: to activate it reboot or execute"
echo "sudo ./loadItNow.sh $GRP"

