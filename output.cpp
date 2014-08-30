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

#include "output.h"
#include "keydefs.h"
#include <QStringList>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>

int OutEvent::fd_kbd;
int OutEvent::fd_mouse;

void OutEvent::generalSetup()
{
	fd_kbd = open("/dev/virtual_kbd", O_WRONLY|O_APPEND);
	fd_mouse = open("/dev/virtual_mouse", O_WRONLY|O_APPEND);
}


/*!
 * @brief  Constructs an output event from a config string, e.g "p+S"
 */
OutEvent::OutEvent(QString s)
{
#ifdef DEBUGME
	initString = s;
#endif
	keycode = 0;
	if (s.startsWith("~"))
	{
		// read macro
		return;
	}
	QStringList l = s.split("+");
	if (l.empty())
		return;
	keycode = keymap[l[0]];
	if (l.length() > 1)
	{
		type = Combo;
		for(unsigned int i = 0; i < l[1].length(); i++)
			if (i >= NUM_MOD)
			{
				qDebug() << "Too many modifiers in " << s;
				break;
			}
			else if (!keymap.contains(l[1].at(i)))
				qDebug() << "Unknown modifier " << l[1][i];
			else
				modifiers.append(keymap[l[1].at(i)]);
		/*if(l[1].contains("~"))
			modifiers = modifiers | MOD_REPEAT;
		if(l[1].contains(""))
			modifiers = modifiers | MOD_MACRO;*/
	}
	else
		type = Simple;
}

void OutEvent::send(int value)
{
	VEvent e[NUM_MOD+1];
	if (type == Simple)
	{
		e[0].type = EV_KEY;
		e[0].code = keycode;
		e[0].value = value;
		if (keycode >= BTN_MISC)
			sendMouseEvent(e);
		else
			sendKbdEvent(e);
	} 
	else if (type == Combo)
	{
		int k = 0;
		if (value != 2)
			for (; k < modifiers.size(); k++)
			{
				e[k].type = EV_KEY;
				e[k].code = modifiers.at(k);
				e[k].value = value;
			}
		e[k].type = EV_KEY;
		e[k].code = keycode;
		e[k].value = value;
		
		if (e[k].code >= BTN_MOUSE)
		{
			sendKbdEvent(e, modifiers.size());
			sendMouseEvent(&e[k], 1);
		}
		else
			sendKbdEvent(e, modifiers.size() + 1);
	}
	else if (type == Macro)
		sendMacro();
	usleep(5000); // wait x * 0.000001 seconds
}

void OutEvent::sendRaw(__s32 type, __s32 code, __s32 value, DType dtype)
{
	VEvent e;
	e.type = type;
	e.code = code;
	e.value = value;
	switch (dtype)
	{
		case Keyboard:
			sendKbdEvent(&e);
			break;
		case Mouse:
			sendMouseEvent(&e);
			break;
		case Auto:
			if (type != EV_KEY || code >= BTN_MOUSE)
				sendMouseEvent(&e);
			else
				sendKbdEvent(&e);
			break;
	};
	
}

void OutEvent::sendMacro()
{

}

/*!
 * @brief Transforms an OutEvent (keycode and maybe modifiers) to a Vector 
 * of raw VEvents and sends it to the virtual keyboard driver.
 * TEvent -> send VEvent[]:
 * @param t Event to be generated.
 */
void OutEvent::send()
{
    /*  
	  if there are modifiers, populate e with a series of keyboard events, 
	  that compose the wanted shortcut.
      Example: (Ctrl+Shift+C) 
      1: Shift down,           ,       ,     ,        , Shift up
      2: Shift down, Ctrl down ,       ,     , Ctrl up, Shift up
      3: Shift down, Ctrl down , C down, C up, Ctrl up, Shift up 
    */
	send(1);
	send(0);
}

/*!
 * @brief sends num raw input events to be generated the virtual mouse device.
 */
//FIXME: should be inline, but then code does not link -> WTF???
void OutEvent::sendMouseEvent(VEvent* e, int num)
{
#ifdef DEBUGME
	if (e->type == EV_KEY)
		qDebug() << "Mouse sending: " 
		<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(fd_mouse, e, num*sizeof(VEvent));
}

/*!
 * @brief sends num raw input events to be generated the virtual keyboard device.
 */
void OutEvent::sendKbdEvent(VEvent* e, int num)
{
#ifdef DEBUGME
	qDebug() << "Kbd sending: " 
	<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(fd_kbd, e, num*sizeof(VEvent));
}

// OutEvent::OutEvent(__s32 code, bool shift, bool alt, bool ctrl)
// {
// 	if (shift)
// 		modifiers.append(KEY_LEFTSHIFT);
// 	if (alt)
// 		modifiers.append(KEY_RIGHTALT);
// 	if (ctrl)
// 		modifiers.append(KEY_LEFTCTRL);
// 	this->keycode = code;
// }

/*!
 * @brief return a QString describing the Output.
 */
QString OutEvent::toString() const
{
	QString s = keymap_reverse[keycode] + "(" + QString::number(keycode) + ")[";
		for(int i = 0; i < modifiers.count(); i++)
		{
			s += keymap_reverse[modifiers[i]] + " (" + QString::number(modifiers[i]) + ")";
			if (i < modifiers.count() - 1)
				s += ", ";
		}
		return s + "]";
}
