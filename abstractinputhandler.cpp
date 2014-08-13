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

#include "abstractinputhandler.h"
#include "inputmangler.h"
#include <fcntl.h>
#include <QDebug>
#include <QTest>

shared_data AbstractInputHandler::sd; // TODO: protect
QMap<QString,QList<AbstractInputHandler*>(*)(QDomNodeList)> AbstractInputHandler::parseMap;

/*!
 * @brief This static Function is called to set up the shared data structure.
 * This includes opening the output devices.
 */
void AbstractInputHandler::generalSetup()
{
	// set up shared data 
	sd.fd_kbd = open("/dev/virtual_kbd", O_WRONLY|O_APPEND);
	sd.fd_mouse = open("/dev/virtual_mouse", O_WRONLY|O_APPEND);
	sd.terminating = false;
#ifdef DEBUGME	
	qDebug() << "kbd: " << sd.fd_kbd << ", mouse: " << sd.fd_mouse;
#endif
}

/*!
 * @brief Add a code to the list of expected inputs. The associated output is set to the same code.
 * @param in Input code.
 * @return Number of expected inputs after the operation.
 */
int AbstractInputHandler::addInputCode(__u16 in)
{
	inputs.append(in);
	outputs.append(OutEvent(in));
	return inputs.size();
}


/*!
 * @brief Add a code to the list of expected inputs.
 * @param in Input code.
 * @param def The default output.
 * @return Number of expected inputs after the operation.
 */
int AbstractInputHandler::addInputCode(__u16 in, OutEvent def)
{
	inputs.append(in);
	outputs.append(def);
	return inputs.size();
}

/*!
 * @brief set current outputs.
 */
void AbstractInputHandler::setOutputs(QVector< OutEvent > o)
{
	if (o.size() != inputs.size())
	{
		qDebug() << "setOutputs: in|out sizes do not match in " << id();
		for (int i = 0; i < inputs.size(); i++)
			qDebug() << "inputs[" << i << "].code = " << inputs.at(i);
		for (int i = 0; i < o.size(); i++)
			qDebug() << "outputs[" << i << "].code = " << o.at(i).code();
	}	
	outputs = o;
}

/*!
 * @brief Transforms an OutEvent (keycode and maybe modifiers) to a Vector 
 * of raw VEvents and sends it to the virtual keyboard driver.
 * TEvent -> send VEvent[]:
 * @param t Event to be generated.
 */
void AbstractInputHandler::sendOutEvent(OutEvent* t)
{
	qDebug() << t->toString();
	VEvent e[NUM_MOD*2+2]; //TODO: limit modifiers
	QVector<__u16> m = t->modifiers;
    /*  
	  if there are modifiers, populate e with a series of keyboard events, 
	  that compose the wanted shortcut.
      Example: (Ctrl+Shift+C) 
      1: Shift down,           ,       ,     ,        , Shift up
      2: Shift down, Ctrl down ,       ,     , Ctrl up, Shift up
      3: Shift down, Ctrl down , C down, C up, Ctrl up, Shift up 
    */
	int offset = 2 + m.size(), k = 0;
	for (; k < m.size(); k++)
	{
		e[k].type = EV_KEY;
		e[k].code = m.at(k);
		e[k].value = 1;
		e[k+offset].type = EV_KEY;
		e[k+offset].code = m.at(k);
		e[k+offset].value = 0;
	}
	e[k].type = EV_KEY;
	e[k].code = t->keycode;
	e[k].value = 1;
	e[k+1].type = EV_KEY;
	e[k+1].code = t->keycode;
	e[k+1].value = 0;
// 	for (int i = 0; i < m.size() * 2 + 2; i++)
// 		qDebug() << e[i].type << " " << e[i].code << " " << e[i].value << " L= " << m.size()*2+2;
	sendKbdEvent(e, m.size() * 2 + 2);
	usleep(5000); // wait x * 0.000001 seconds
}

/*!
 * @brief sends num raw input events to be generated the virtual mouse device.
 */
//FIXME: should be inline, but then code does not link -> WTF???
void AbstractInputHandler::sendMouseEvent(VEvent* e, int num) 
{
#ifdef DEBUGME
	if (e->type == EV_KEY)
		qDebug() << "Mouse sending: " 
		<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(sd.fd_mouse, e, num*sizeof(VEvent));
}

/*!
 * @brief sends num raw input events to be generated the virtual keyboard device.
 */
void AbstractInputHandler::sendKbdEvent(VEvent* e, int num)
{
	
#ifdef DEBUGME
	qDebug() << "Kbd sending: " 
	<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(sd.fd_kbd, e, num*sizeof(VEvent));
}

/*!
 * @brief Register a static parser function.
 * @param id Id used in config.xml to identify the subclass of AbstractInputHandler
 * @param func Function used to parse all <id>xml parts.
 */
void AbstractInputHandler::registerParser(QString id, QList<AbstractInputHandler*>(*func)(QDomNodeList))
{
	parseMap[id] = func;
}

/* 
 * parses /proc/bus/input/devices for relevant information, where relevant is:
 * I: Vendor=1395 Product=0020
 * H: Handlers=kbd event9     <-- kbd or mouse? which event file in /dev/input/ is it?
 */
QList< AbstractInputHandler::idevs > AbstractInputHandler::parseInputDevices()
{
	QFile d("/proc/bus/input/devices");
	
	if(!d.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not open " << d.fileName();
		return QList<idevs>();
	}
	QTextStream t(&d);
	// we read it line by line
	QStringList devs = t.readAll().split("\n"); 
	QList<idevs> l;
	idevs i;
	QStringList tmp;
	int idx;
	
	QList<QString>::iterator li = devs.begin();
	while (li != devs.end())
	{
		// "I:" marks the beginning of a new section
		// and also contains information we want
		if(!(*li).startsWith("I"))
		{
			++li;
			continue;
		}
		tmp = (*li).split(" ");
		idx = tmp.indexOf(QRegExp("Vendor.*"));
		i.vendor = tmp.at(idx).right(4);
		idx = tmp.indexOf(QRegExp("Product.*"));
		i.product = tmp.at(idx).right(4);
		while (li != devs.end())
		{
			// "H:" marks the line with the rest
			if(!(*li).startsWith("H"))
			{
				++li;
				continue;
			}
			tmp = (*li).split(QRegExp("[\\s=]"));
			idx = tmp.indexOf(QRegExp("event.*"));
			i.event = tmp.at(idx);
			i.mouse = (tmp.indexOf(QRegExp("mouse.*")) != -1);
			++li;
			break;
		}
		l.append(i);
	}
	return l;
}



