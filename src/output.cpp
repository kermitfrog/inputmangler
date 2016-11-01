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
#include <QTest>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

int OutEvent::fds[5];

/*!
 * @brief opens virtual devices for output
 */
void OutEvent::generalSetup()
{
	openVDevice("/dev/virtual_kbd", 1);
	openVDevice("/dev/virtual_mouse", 2);
	openVDevice("/dev/virtual_tablet", 3);
	openVDevice("/dev/virtual_joystick", 4);
#ifdef DEBUGME	
	qDebug() << "kbd: " << OutEvent::fds[1] << ", mouse: " << OutEvent::fds[2];
	qDebug() << "tablet: " << OutEvent::fds[4] << ", joystick: " << OutEvent::fds[4];
#endif
}

/*!
 * @brief opens a virtual device for output.
 */
void OutEvent::openVDevice(char* path, int num)
{
	int fd = -1, numtries = 100;
	while (fd < 0 && numtries--)
	{
		fd = open(path, O_WRONLY|O_APPEND);
	}
	fds[num] = fd;
	//qDebug() << path << ": errno = " << errno << "  tries left before giving up: " << numtries;

	if (fd < 0)
		QString message = "Could not open device " + QString::fromUtf8(path) + ", Error: " + QString::fromUtf8(strerror(errno));
}

/*!
 * @brief closes virtual devices
 */
void OutEvent::closeVirtualDevices()
{
	for (int i = 1; i <= 4; i++)
		close(fds[i]);
}



/*!
 * @brief  Constructs an output event from a config string, e.g "p+S"
 */
OutEvent::OutEvent(QString s)
{
#ifdef DEBUGME
	initString = s;
#endif
	eventcode = 0;
	if (s.startsWith("~"))
	{
		int leftBrace = s.indexOf('(') + 1;
		int rightBrace = s.lastIndexOf("~)");
		QString type = s.left(leftBrace);
		if (type == "~Seq(" || type == "~Sequence(" || type == "~Macro(" )
		{
			outType = Macro;
			QStringList l = s.mid(leftBrace, rightBrace - leftBrace).split(",");
			if (l.empty())
				return;
			parseMacro(l);
		}
		else if (type == "~A(" || type == "~Auto(" )
		{
			outType = Repeat;
			fromInputEvent(keymap[s.mid(leftBrace, rightBrace - leftBrace)]);
			customValue = 0;
		}
		return;
	}
	QStringList l = s.split("+");
	if (l.empty())
		return;
	fromInputEvent(keymap[l[0]]);
	if (l.length() > 1)
	{
		outType = Combo;
		parseCombo(l);
	}
	else
		outType = Simple;
}

/*!
 * @brief  Deconstructs an output event. Deletes children if neccessary.
 */
OutEvent::~OutEvent()
{
	if (next != nullptr)
		delete static_cast<OutEvent*>(next);
}

void OutEvent::parseMacro(QStringList l)
{
	QString s = l.takeFirst().trimmed();
	if (s.isEmpty())
		return;
	if (!s.startsWith('~'))
	{
		outType = OutEvent::Macro;
		QStringList parts = s.split(' ', QString::SkipEmptyParts);
		QStringList comboParts = parts[0].split("+");
		fromInputEvent(keymap[comboParts[0]]);
		if (comboParts.size() > 1)
			parseCombo(comboParts);
		if (parts.size() > 1)
		{
			hasCustomValue = true;
			customValue = parts[1].toInt();
		}
	}
	else if (s.startsWith("~s"))
	{
		outType = OutEvent::Wait;
		customValue = s.mid(2).toInt()*1000;
	}
	else
		qDebug() << "parseMacro: unsupported operation: " << s;
	if (l.count())
	{	next = new OutEvent(l);
		OutEvent *o = static_cast<OutEvent*>(next);
	}
}

OutEvent::OutEvent(QStringList macroParts)
{
	parseMacro(macroParts);
}

void OutEvent::parseCombo(QStringList l)
{
	for(unsigned int i = 0; i < l[1].length(); i++)
		if (i >= NUM_MOD)
		{
			qDebug() << "Too many modifiers in " << l;
			break;
		}
		else if (!keymap.contains(l[1].at(i)))
			qDebug() << "Unknown modifier " << l[1][i];
		else
			modifiers.append(keymap[l[1].at(i)].code);
	/*if(l[1].contains("~"))
		modifiers = modifiers | MOD_REPEAT;
	if(l[1].contains(""))
		modifiers = modifiers | MOD_MACRO;*/
}

/*!
 * @brief initialize Values from an InputEvent
 */
void OutEvent::fromInputEvent(InputEvent& e)
{
	eventtype = e.type;
	valueType = e.valueType;
	eventcode = e.code;
}

/*!
 * @brief construct from an InputEvent
 */
OutEvent::OutEvent(InputEvent& e)
{
	outType = Simple;
	fromInputEvent(e);
}

/*!
 * @brief Sends the OutEvent to the correct Output device.
 * If valueType is set Positive or Negative, the value will be multiplied by -1 if neccessary.
 * Calls send or send(int).
 * @param value The value of the input event (see linux/input.h). 
 * @param sourceType The type of device, this was triggered from.
 */
void OutEvent::send(int value, __u16 sourceType)
{
	if (eventtype == sourceType)
	{
		if (eventtype == EV_KEY || valueType == All)
		{
			send(value);
		}
		else
		{
			if ( (valueType == Positive && value < 0) 
				|| (valueType == Negative && value > 0) )
			{
				send(value * -1);
			}
			else
			{
				send(value);
			}
		}
	}
	else if (sourceType == EV_REL && eventtype == EV_KEY)
	{
		send();
	}
	else if (sourceType == EV_KEY && eventtype == EV_REL)
	{
		if (value == 0)
			return;
		if (valueType == Negative)
			send(-1);
		else
			send(1);
	}
}

/*!
 * @brief Sends an OutEvent with a value. 
 * send(i) for a Simple event will just send the event with the value.
 * send(i) for a Combo event will also triggered all modifiers before/after the target 
 * key and split output between mouse and keyboard when neccessary.
 * @param value The value of the input event (see linux/input.h). 
 */
void OutEvent::send(int value)
{
	switch (outType)
	{
		case OutEvent::Simple:
			sendSimple(value);
			break;
		case OutEvent::Combo:
			sendCombo(value);
			break;
		case OutEvent::Repeat:
			switch(value)
			{
				case 1:
					sendSimple(1);
					customValue = 0;
					break;
				case 2:
					sendSimple(customValue);
					customValue ^= 1;
					break;
				default:
					sendSimple(0);
			}
			break;
		case OutEvent::Macro:
			if (value == 1)
			{
				sendMacro();
				proceed();
			}
			break;
		case OutEvent::Wait:
			if (value == 1)
			{
				usleep(customValue); //TODO: make it non-blocking!
				proceed();
			}
			break;
	};
			
// 	usleep(5000); // wait x * 0.000001 seconds
}

void OutEvent::sendSimple(int value)
{
	VEvent e[NUM_MOD+1];
	e[0].type = eventtype;
	e[0].code = eventcode;
	e[0].value = value;
	if (eventcode >= BTN_MISC)
		sendMouseEvent(e);
	else
		sendEvent(e);
}

void OutEvent::sendCombo(int value)
{
	VEvent e[NUM_MOD+1];
	int k = 0;
	if (value != 2)
		for (; k < modifiers.size(); k++)
		{
			e[k].type = EV_KEY;
			e[k].code = modifiers.at(k);
			e[k].value = value;
		}
	e[k].type = EV_KEY;
	e[k].code = eventcode;
	e[k].value = value;
	
	if (e[k].code >= BTN_MOUSE)
	{
		sendKbdEvent(e, modifiers.size());
		sendMouseEvent(&e[k], 1);
	}
	else
		sendKbdEvent(e, modifiers.size() + 1);
}


/*!
 * @brief Sends a raw event to the device corrosponding to dtype.
 * @param type event type - see linux/input.h
 * @param code event code - see linux/input.h
 * @param value event value - see linux/input.h
 * @param dtype Type of the device on which the event should be generated.
 */
void OutEvent::sendRaw(__s32 type, __s32 code, __s32 value, DType dtype)
{
// 	qDebug() << "sendRaw: type= " << type << ", code= " << code << ", value= " << value 
// 			 << ", dtype= " << dtype;
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
			if (code >= BTN_MOUSE)
				sendMouseEvent(&e);
			else
				sendEvent(&e);
			break;
		case Tablet:
			write(fds[3], &e, sizeof(VEvent));
			break;
		case Joystick:
			write(fds[4], &e, sizeof(VEvent));
			break;
		case TabletOrJoystick:
			write(fds[3], &e, sizeof(VEvent));
			write(fds[4], &e, sizeof(VEvent));
			break;
	};
}

/*!
 * @brief Not yet implemented.
 */
void OutEvent::sendMacro()
{
	if (!hasCustomValue)
	{
		sendCombo(1);
		sendCombo(0);
	}
	else 
		sendSimple(customValue);
}

/*!
 * @brief go to next part of macro
 */
void OutEvent::proceed()
{
	if (next && outType != OutEvent::Custom)
		static_cast<OutEvent*>(next)->send(1);
}

/*!
 * @brief Triggers the OutEvent, as in press and release.
 * calls send(1), then send(0) followed by a 5 microsecond sleep.
 * Mostly used by Net module.
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
	usleep(5000); // wait x * 0.000001 seconds
}

/*!
 * @brief Sends num VEvents to the virtual output determined by the event type.
 * EV_KEY -> Keyboard
 * EV_REL -> Mouse
 * EV_ABS -> Tablet
 * @param e buffer with VEvents
 * @param num number of VEvents
 */
void OutEvent::sendEvent(OutEvent::VEvent* e, int num)
{
	write(fds[e[0].type], e, num*sizeof(VEvent));
}

/*!
 * @brief sends num raw input events to be generated the virtual mouse device.
 * @param e buffer with VEvents
 * @param num number of VEvents
 */
//FIXME: should be inline, but then code does not link -> WTF???
void OutEvent::sendMouseEvent(VEvent* e, int num)
{
#ifdef DEBUGME
		qDebug() << "Mouse sending: " 
		<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(fds[2], e, num*sizeof(VEvent));
}

/*!
 * @brief sends num raw input events to be generated the virtual keyboard device.
 * @param e buffer with VEvents
 * @param num number of VEvents
 */
void OutEvent::sendKbdEvent(VEvent* e, int num)
{
#ifdef DEBUGME
	qDebug() << "Kbd sending: " 
	<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(fds[1], e, num*sizeof(VEvent));
}

/*!
 * @brief return a QString describing the Output.
 */
QString OutEvent::toString() const
{
	int searchcode = eventcode;
	if (eventtype != EV_KEY)
		searchcode = eventcode+(10000*eventtype)+(1000*valueType);
	QString s = keymap_reverse[searchcode] + "(" + QString::number(searchcode) + ")[";
		for(int i = 0; i < modifiers.count(); i++)
		{
			s += keymap_reverse[modifiers[i]] + " (" + QString::number(modifiers[i]) + ")";
			if (i < modifiers.count() - 1)
				s += ", ";
		}
		return s + "], Type = " + QString::number(outType);
}

OutEvent& OutEvent::operator=(const OutEvent& other)
{
	eventtype = other.eventtype;
	eventcode = other.eventcode;
	outType = other.outType;
	valueType = other.valueType;
	hasCustomValue = other.hasCustomValue;
	customValue = other.customValue;
	modifiers = other.modifiers;
	if (other.next)
		next = new OutEvent(*static_cast<OutEvent*>(other.next));
	return *this;
}

OutEvent::OutEvent(const OutEvent& other)
{
	operator=(other);
}

OutEvent::OutEvent(OutEvent& other)
{
	operator=(other);
}


