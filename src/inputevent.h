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

#pragma once
#include <linux/input.h>
#include <QString>
#include "definitions.h"

/*!
 * @brief Class containing an input event.
 */
class InputEvent
{
	
public:
    InputEvent();
    InputEvent(const InputEvent& other);
    InputEvent(__u16 code, __u16 type = EV_KEY, ValueType valueType = All, int min = 0, int max = 0)
				: code(code), type(type), valueType(valueType), absmin(min), absmax(max) {};
    ~InputEvent();
    InputEvent& operator=(const InputEvent& other);
    bool operator==(const InputEvent& other);
    bool operator==(const input_event& other);
    bool operator==(const __u16 other) {return other == code;};
	QString print();

	__u16 type = EV_KEY;        //!< event type (see linux/input.h)
	__u16 code = 0;             //!< event code (see linux/input.h) 
	ValueType valueType = All;  //!< filter for values (see definitions.h)
	int absmin;
	int absmax;

	void setInputEvent(input_event * ev, __s32 value);
};

