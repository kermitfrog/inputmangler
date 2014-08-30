/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Arek <arek@ag.de1.cc>
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
enum ValueType {All, Positive, Negative, Zero};

class InputEvent
{
	
public:
    InputEvent();
    InputEvent(const InputEvent& other);
    InputEvent(__u16 code, __u16 type = EV_KEY, ValueType valueType = All)
				: code(code), type(type), valueType(valueType) {};
    ~InputEvent();
    InputEvent& operator=(const InputEvent& other);
    bool operator==(const InputEvent& other);
    bool operator==(const input_event& other);
    bool operator==(const __u16 other) {return other == code;};
	QString print();
	
	__u16 type = EV_KEY;
	__u16 code = 0;
	ValueType valueType = All;
	
	
};

