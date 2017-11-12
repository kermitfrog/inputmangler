#!/bin/bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
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

echo ""
echo "which user group should have rights to access the input devices? (it will be created if neccessary) [inputmangler]"
read GRP
if [[ "$GRP" == "" ]] ; then
	GRP=inputmangler
fi

echo "Done!"
echo "if your kernel is updated, you will need to reinstall inputdummy. Do this by going into the inputdummy directory and executing"
echo "make && make install"
echo ""

scripts/make_udev_rules.sh $GRP

