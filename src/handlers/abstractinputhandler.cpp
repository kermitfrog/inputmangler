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
#include "definitions.h"
#include <QDebug>
#include <QTest>

shared_data AbstractInputHandler::sd; // TODO: protect?
QMap<QString,QList<AbstractInputHandler*>(*)(pugi::xml_node&)> AbstractInputHandler::parseMap;

/*!
 * @brief This static Function is called to set up the shared data structure.
 */
void AbstractInputHandler::generalSetup()
{
	// set up shared data
	sd.terminating = false;
}

/*!
 * @brief Add an input event to the list of expected inputs. The associated output is set to the same code.
 * @param in Input event.
 * @return Number of expected inputs after the operation.
 */
int AbstractInputHandler::addInput(InputEvent in)
{
	inputs.append(in);
	outputs.append(OutEvent(in));
	return inputs.size();
}

/*!
 * @brief Add an input event to the list of expected inputs.
 * @param in Input event.
 * @param def The default output.
 * @return Number of expected inputs after the operation.
 */
int AbstractInputHandler::addInput(InputEvent in, OutEvent def)
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
			qDebug() << "inputs[" << i << "].code = " << inputs.at(i).code;
		for (int i = 0; i < o.size(); i++)
			qDebug() << "outputs[" << i << "].code = " << o.at(i).code();
	}	
	outputs = o;
}

/*!
 * @brief Register a static parser function.
 * @param id Id used in config.xml to identify the subclass of AbstractInputHandler
 * @param func Function used to parse all <id>xml parts.
 */
void AbstractInputHandler::registerParser(QString id, QList< AbstractInputHandler* >(*func)(pugi::xml_node&))
{
	parseMap[id] = func;
}

/* 
 * parses /proc/bus/input/devices for relevant information, where relevant is:
 * I: Vendor=1395 Product=0020
 * P: Phys=usb-0000:00:1d.0-1/input3
 * H: Handlers=kbd event9     <-- kbd, mouse or js? which event file in /dev/input/ is it?
 * B: ABS=    <-- does it have absolute axes?
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
	// we parse it line by line
	QStringList devs = t.readAll().split("\n"); 
	QList<idevs> l;
	idevs i;
	QStringList tmp;
	int idx;
	
	QList<QString>::iterator li = devs.begin();
	while (li != devs.end())
	{
		//   Handlers=js, B:ABS=.*   , Handlers=mouse
		bool js = false , abs = false, mouse = false;
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
			if ((*li).startsWith("P:"))
				i.phys = (*li).mid(8);
			else if ((*li).startsWith("H:"))
			{
				tmp = (*li).split(QRegExp("[\\s=]"));
				idx = tmp.indexOf(QRegExp("event.*"));
				i.event = tmp.at(idx);
				mouse = (tmp.indexOf(QRegExp("mouse.*")) != -1);
				js = (tmp.indexOf(QRegExp("js.*")) != -1);
			}
			else if ((*li).startsWith("B: ABS="))
				abs = true;
			else if ((*li).isEmpty())
				break;
			++li;
		}
		if (js && mouse)
			i.type = TabletOrJoystick;
		else if (js)
			i.type = Joystick;
		else if (abs)
			i.type = Tablet;
		else if (mouse)
			i.type = Mouse;
		else
			i.type = Keyboard;
		l.append(i);
	}
	return l;
}

/*!
 * @brief Reads attributes of idevs to values in XML element at attr
 */
void AbstractInputHandler::idevs::readAttributes(pugi::xml_node attr)
{
	vendor  = attr.attribute("vendor").value();
	product = attr.attribute("product").value();
	id      = attr.attribute("id").value();
	phys    = attr.attribute("phys").value();
}

/*!
 * @brief Returns true if o is a match to the current objects attributes.
 * Implemented for use in QList::count(), QList::indexOf().
 */
bool AbstractInputHandler::idevs::operator==(AbstractInputHandler::idevs o) const
{
	return (phys == o.phys || ( vendor == o.vendor && product == o.product) );
}

/*!
 * @brief Returns a map of keycodes to the corresponding index in inputs
 * @return QMap<keycode, index in inputs>
 */
QMap<__u16,int> AbstractInputHandler::getInputMap() {
    QMap<__u16,int> map;
    for(int i = 0; i < inputs.size(); i++)
        map[inputs[i].code] = i;
	return map;
}

