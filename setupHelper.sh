#!/bin/bash
CONFIG_PATH="$HOME/.config/inputMangler/"
SOURCE_PATH="$PWD"

mkdir -p $CONFIG_PATH > /dev/null
cd $SOURCE_PATH

echo "Copy example config file to $CONFIG_PATH ?  [y|N]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' ]]; then
	cp doc/config.xml.example $CONFIG_PATH/config.xml
	cp keymaps/axismap $CONFIG_PATH/axismap
fi

echo ""
echo "Which keymaps should be made your default? (copied to $CONFIG_PATH)"
echo "Available:"
for var in `ls keymaps/keymap.* | cut -b "16-"`
do
	echo $var
done
read LAYOUT
cp -v keymaps/keymap.$LAYOUT $CONFIG_PATH/keymap
cp -v keymaps/charmap.$LAYOUT $CONFIG_PATH/charmap

cd -
