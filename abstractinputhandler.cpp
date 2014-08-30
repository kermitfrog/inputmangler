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
	sd.terminating = false;
#ifdef DEBUGME	
	qDebug() << "kbd: " << OutEvent::fd_kbd << ", mouse: " << OutEvent::fd_mouse;
#endif
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



