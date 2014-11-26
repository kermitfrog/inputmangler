#!/bin/bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src/
make

if [[ $? -gt 0 ]] ; then
	exit 1
fi

echo "inputmangler can run from any directory, so it does not need to be installed system wide - but that doesn't mean you can't"
echo "install to /usr/local/?  [y|N]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' ]]; then
	sudo make install
fi

cd ../inputdummy
make
echo "inputdummy however is a kernel module and should be installed system wide. Do it? [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == "" ]]; then
	sudo make install
fi
cd ..

echo "Append inputdummy to the list of modules that are loaded on boot?  [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == "" ]]; then
	echo inputdummy | sudo tee -a /etc/modules > /dev/null
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
	sudo sed -ie "s/^exit 0$/chown :$GRP \/dev\/virtual_kbd \/dev\/virtual_mouse\nchmod 660 \/dev\/virtual_kbd \/dev\/virtual_mouse\nexit 0/" /etc/rc.local
fi

echo "Done!"
echo "if your kernel is updated, you will need to reinstall inputdummy. Do this by going into the inputdummy directory and executing"
echo "make && make install"
echo ""

scripts/make_udev_rules.sh $GRP

