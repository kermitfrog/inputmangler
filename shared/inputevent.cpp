/*
 * This file is part of inputmangler, a programm which intercepts and
 * transforms linux input events, depending on the active window.
 * Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "inputevent.h"
#include <QDebug>

/*!
 * @brief Default constructor
 */
InputEvent::InputEvent()
{

}

/*!
 * @brief Copy constructor
 */
InputEvent::InputEvent(const InputEvent& other)
{
	type = other.type;
	code = other.code;
	valueType = other.valueType;
    absmin = other.absmin;
    absmax = other.absmax;
}

/*!
 * @brief Default destructor
 */
InputEvent::~InputEvent()
{

}

/*!
 * @brief Copy operator
 */
InputEvent& InputEvent::operator=(const InputEvent& other)
{
	type = other.type;
	code = other.code;
	valueType = other.valueType;
    absmin = other.absmin;
	absmax = other.absmax;
	return *this;
}

/*!
 * @brief Compares with another InputEvent.
 */
bool InputEvent::operator==(const InputEvent& other)
{
    return (type == other.type && code == other.code && valueType == other.valueType
			&& ( type != EV_ABS || ( absmin == other.absmin && absmax == other.absmax)));
}

/*!
 * @brief Compares with a linux input event (see linux/input.h)
 */
bool InputEvent::operator==(const input_event& other)
{
// 	qDebug() << "type = " << type << ", other type = " << other.type;
	if (type != other.type || code != other.code)
		return false;
	if (type == EV_REL)
		switch (valueType){
			case All:
			case TabletAxis:
			case JoystickAxis:
				return true;
			case Positive:
				return other.value > 0;
			case Negative:
				return other.value < 0;
			case Zero:
				return other.value == 0;
		};
	return true;
}

/*!
 * @brief Print InputEvent to a QString.
 * @return the formated QString.
 */
QString InputEvent::print()
{
	return "InputEvent: code = " + QString::number(code) + ", type = " + QString::number(type)
				+ ", valueType = " + QString::number(valueType);
}

/*!
 * Sets values of an input_event to those defined in this Object and value
 *
 * @param ev pointer to target
 * @param value value to set
 */
void InputEvent::setInputEvent(input_event * ev, __s32 value) {
	ev->type = type;
	ev->code = code;
	ev->value = value;
}

/*!
 * Returns number of uinput device appropriate for this event
 * @return 1: Keyboard, 2: Mouse, 3: Tablet, 4: Joystick
 */
__u8 InputEvent::getFd() const {
	switch (type) {
		case EV_KEY:
			if (code < BTN_MOUSE) // Basic Keyboard Keys and Misc Buttons
				return 1;
            else if (code < BTN_JOYSTICK) // Mouse Buttons
                return 2;
			else if (code < BTN_DIGI) // Basic Joystick and Gamepad Buttons
				return 4;
			else if (code < KEY_OK) // Tablet Buttons
				return 3;
			else if (code < BTN_DPAD_UP) // Extra Keyboard Keys
				return 1;
			else if (code < KEY_ALS_TOGGLE) // DPAD (this is joystick stuff, right?)
				return 4;
			else if (code < BTN_TRIGGER_HAPPY) // More Extra Keyboard Keys
				return 1;
			else               // Extra Buttons
				return 4;
		case EV_REL:
			return 2;
		case EV_ABS:
			if (valueType == JoystickAxis)
				return 4;
			else
				return 3;
		default:
			return 0;
	}
}

__u16 InputEvent::sourceType() const {
	if (valueType == Negative)
		return type + NegativeModifier;
	return type;
}



