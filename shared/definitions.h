/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <linux/input-event-codes.h>

/*!
 * @brief Device Type.
 */
enum DType{
	Auto, 			 //!< Auto Detect - currently not used
	Keyboard = 1, 	 //!< Keyboard - a device that sends only keys and may have LEDs
	Mouse = 2,		 //!< Mouse - a device that sends keys and relative movements
	Tablet = 3, 	 //!< Tablet or Touchscreen - a device that sends special keys and absolute movements.
	Joystick = 4, 	 //!< Joystick - a device that sends keys and absolute movements.
	TabletOrJoystick //!< A device that can't be clearly distinguished between tablet or joystick (eg VirtualBox Tablet)
};


/*!
 * @brief ValueType: which values should be matched.
 */
enum ValueType {
	All, 			//!< all values
	Positive, 		//!<  >0 (e.g. mouse wheel down)
	Negative, 		//!<  <0 (e.g. mouse wheel up)
	Zero, 			//!< ==0 currently not used
	TabletAxis, 	//!< axis of a tablet device
	JoystickAxis	//!< axis of a joystick device
};

const int NUM_INPUTBITS = EV_CNT + 2; // +2 for EVBITS & ABS_Joystick
const int EV_ABSJ = EV_CNT + 1;

const __u16 NegativeModifier = 8;
