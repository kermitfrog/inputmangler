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
#include "definitions.h"
#include <linux/input.h>
#include <QVector>
#include <unistd.h> //@TODO: still needed here?
#include <pugixml.hpp>

/*!
 * @brief Data shared by all Handlers
 */
struct shared_data
{
	bool terminating;
	QMap<QString, void*> infoCache;
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
		QString vendor;  //!< 4-Hex-Digit Vendor id
		QString product; //!< 4-Hex-Digit Product id
		QString phys;    //!< Phys path
		QString event;   //!< Event file in /dev/input
		QString id;      //!< id set in config.xml
		DType type;      //!< Device Type
		bool operator==(idevs o) const;
        void readAttributes(pugi::xml_node node);
    };

public:
	virtual ~AbstractInputHandler() {};;
	virtual QString id() const {return _id;};
	virtual void setOutputs(QVector<OutEvent> o);
	virtual QVector<OutEvent> getOutputs() const {return outputs;};;
	int getNumInputs() const {return inputs.size();};
	virtual QMap<__u16,int> getInputMap();
	int getNumOutputs() const {return outputs.size();};
	bool hasWindowSpecificSettings() const {return _hasWindowSpecificSettings;};
	static void registerParser(QString id, QList<AbstractInputHandler*>(*func)(pugi::xml_node&));
	static void generalSetup();
	static shared_data sd; // TODO: protect?
	static QMap<QString,QList<AbstractInputHandler*>(*)(pugi::xml_node&)> parseMap;
    virtual int getType() = 0;
	int getInputIndex(QString key) const ;
	virtual input_absinfo ** setInputCapabilities(QBitArray *inputBits[]) {};

signals:
	void windowChanged(QString wclass, QString title);
	void windowTitleChanged(QString wclass);
	
protected:
	QString _id; // id set in config.xml
	QVector<OutEvent> outputs;	// current target events
	QVector<InputEvent> inputs;	// input events to be transformed
	virtual int addInput(InputEvent in);
	virtual int addInput(InputEvent in, OutEvent def);
	bool _hasWindowSpecificSettings;
	static QList<idevs> parseInputDevices();

};

