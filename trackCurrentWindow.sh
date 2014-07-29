#!/bin/bash
FREQUENCY=2 # check every x seconds

while [[ /bin/true ]] 
do
	WNAME=$(xprop -id `xdotool getactivewindow` | grep WM_NAME | cut -d\" -f 2 | head -n1)
	WCLASS=$(xprop -id `xdotool getactivewindow` | grep WM_CLASS | cut -d\" -f 4 | head -n1)
	if [[ $LASTWNAME != $WNAME || $LASTWCLASS != $WCLASS ]] ; then
		qdbus org.inputManglerInterface /org/inputMangler/Interface org.inputMangler.API.Interface.activeWindowChanged "$WCLASS" "$WNAME"
		LASTWNAME=$WNAME
		LASTWCLASS=$WCLASS
	fi
	sleep $FREQUENCY
done


