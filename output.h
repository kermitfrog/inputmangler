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

#include <linux/input.h> //__u16...
#include <QString>
#include <QVector>
#include <QHash>


enum DType{Auto, Keyboard, Mouse};

/*!
 * @brief An output event, e.g: 
 * "S" for press shift
 * "g" for press g
 * "d+C" for press Ctrl-D
 */
class OutEvent 
{
	enum OutType {Simple, Combo, Macro, Custom};
	/*!
	* @brief Data that wil be sent to inputdummy, aka low level input event
	* See linux/input.h for Details on Variables.
	*/
	struct VEvent
	{
		__s32 type; // these two are __s16 in input.h, but input_event(), called in
		__s32 code; // inputdummy expects int
		__s32 value;
	};
	friend class WindowSettings;
	friend class TransformationStructure;
public:
	OutEvent() {};
	OutEvent(int c) {keycode = c;};
	OutEvent(QString s);
// 	OutEvent(__s32 code, bool shift, bool alt = false, bool ctrl = false); 
	QString toString() const;
	QVector<__u16> modifiers;
	__u16 keycode;
	__u16 code() const {return keycode;};
	OutType type;
	void send();
	void send(int value);
	// protected?
	static void sendMouseEvent(VEvent *e, int num = 1);
	static void sendKbdEvent(VEvent *e, int num = 1);
	
	static void sendRaw(__s32 type, __s32 code, __s32 value, DType dtype = Auto);
	
	static void generalSetup();
	static int fd_kbd;
	static int fd_mouse;
#ifdef DEBUGME
	QString initString;
#endif
protected:
	void sendMacro();
	
};



