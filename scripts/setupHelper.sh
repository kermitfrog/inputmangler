#!/bin/bash
CONFIG_PATH="$HOME/.config/inputMangler/"
DOC_PATH="$PWD/doc"
KEYMAP_PATH="$PWD/keymaps"
if [ -d /usr/local/inputmangler ] ; then
	DOC_PATH="/usr/local/inputmangler/doc"
	KEYMAP_PATH="/usr/local/inputmangler/keymaps"
fi
if [ -d /usr/share/inputmangler ] ; then
	DOC_PATH="/usr/share/doc/inputmangler"
	KEYMAP_PATH="/usr/share/inputmangler/keymaps"
fi

mkdir -p $CONFIG_PATH > /dev/null

echo "Copy example config file to $CONFIG_PATH ?  [y|N]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' ]]; then
	cp $DOC_PATH/config.xml.example $CONFIG_PATH/config.xml
fi

echo ""
echo "Which keymaps should be made your default? (copied to $CONFIG_PATH)"
echo "Available:"
for var in `ls $KEYMAP_PATH/keymap.* | sed "s/^.*\\.//"`
do
	echo $var
done
read LAYOUT
cp -v $KEYMAP_PATH/keymap.$LAYOUT $CONFIG_PATH/keymap
cp -v $KEYMAP_PATH/charmap.$LAYOUT $CONFIG_PATH/charmap
cp -v $KEYMAP_PATH/axismap $CONFIG_PATH/axismap

if [ ! -e "/usr/share/kde4/apps/kwin/scripts/contents/code/main.js" ]; then
	echo "install notification script for the KDE window manager? [Y|n]"
	read INP
	if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == '' ]]; then
		plasmapkg --type kwinscript -i kwinscript/kwinNotifyOnWindowChange.kwinscript
	fi
fi


echo "If you are using KDE, you should enable the notification script for the KDE window manager?"
echo "Open the control module now? [Y|n]"
read INP
if [[ "$INP" == 'y' || "$INP" == 'y' ||  "$INP" == 'yes'  || "$INP" == 'Yes' || "$INP" == '' ]]; then
	kcmshell4 kwinscripts > /dev/null &
fi

