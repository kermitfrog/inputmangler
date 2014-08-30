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

struct stat;
class OutEvent;
class TransformationStructure;

#include <QThread>
#include "inputmangler.h"
#include "inputevent.h"
#include <linux/input.h>
#include <QVector>
#include <unistd.h> //@TODO: still needed here?
#include <QtXml>

/*!
 * @brief Data shared by all Handlers
 */
struct shared_data
{
	bool terminating;
};



/*!
 * @brief base class for event transformation
 */
class AbstractInputHandler : public QThread
{
	Q_OBJECT
	// structure with information on input devices as read
	// from /proc/bus/input/devices
protected:
	class idevs 
	{
	public:
		QString vendor;
		QString product;
		QString event;
		QString id;
		bool mouse; // TODO: make an enum -> more possible types
		bool operator==(idevs o) const{
			return (vendor == o.vendor && product == o.product);
		};
	};
	
public:
	virtual ~AbstractInputHandler() {};
	virtual void setId(QString i) {_id = i;};
	virtual QString id() const {return _id;};
	virtual void setOutputs(QVector<OutEvent> o);
	virtual QVector<OutEvent> getOutputs() const {return outputs;};
// 	static QList<AbstractInputHandler*> parseXml(QDomNode nodes) {};
	int inputIndex(QString s) const {return inputs.indexOf(InputEvent(keymap[s]));};
	int getNumInputs() const {return inputs.size();};
	int getNumOutputs() const {return outputs.size();};
	bool hasWindowSpecificSettings() const {return _hasWindowSpecificSettings;};
	static void registerParser(QString id, QList<AbstractInputHandler*>(*func)(QDomNodeList));
	static void generalSetup();
	static shared_data sd; // TODO: protect?
	static QMap<QString,QList<AbstractInputHandler*>(*)(QDomNodeList)> parseMap;

signals:
	void windowChanged(QString wclass, QString title);
	void windowTitleChanged(QString wclass);
	
protected:
	QString _id;
	QVector<OutEvent> outputs;	// current target events
	QVector<InputEvent> inputs;	// input events to be transformed
	virtual int addInput(InputEvent in);
	virtual int addInput(InputEvent in, OutEvent def);
	bool _hasWindowSpecificSettings;
	static QList<idevs> parseInputDevices();
	
};

