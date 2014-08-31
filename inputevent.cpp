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

InputEvent::InputEvent()
{

}

InputEvent::InputEvent(const InputEvent& other)
{
	type = other.type;
	code = other.code;
	valueType = other.valueType;
}

InputEvent::~InputEvent()
{

}

InputEvent& InputEvent::operator=(const InputEvent& other)
{
	type = other.type;
	code = other.code;
	valueType = other.valueType;
}

bool InputEvent::operator==(const InputEvent& other)
{

}

bool InputEvent::operator==(const input_event& other)
{
// 	qDebug() << "type = " << type << ", other type = " << other.type;
	if (type != other.type || code != other.code)
		return false;
	if (type == EV_REL)
		switch (valueType){
			case All:
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

QString InputEvent::print()
{
	return "InputEvent: code = " + QString::number(code) + ", type = " + QString::number(type)
				+ ", valueType = " + QString::number(valueType);
}

