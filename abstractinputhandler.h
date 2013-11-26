/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Arek <arek@ag.de1.cc>

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


#ifndef ABSTRACTINPUTHANDLER_H
#define ABSTRACTINPUTHANDLER_H
class OutEvent;
class TransformationStructure;

#include <QThread>
#include "inputmangler.h"
#include <linux/input.h>
#include <QVector>
#include <unistd.h>

struct shared_data
{
	int fd_kbd;
	int fd_mouse;
	bool terminating;
};

struct VEvent
{
//	__u16 type; //should be 16... TODO: fix in inputdummy
//	__u16 code;
	__s32 type;
	__s32 code;
	__s32 value;
};

class TEvent
{
public:
	TEvent(){TEvent(0);};
	TEvent(__s32 c, bool s = false, bool a = false, bool C = false);
	QVector<__u16> modifiers;
	__s32 code;
};

class AbstractInputHandler : public QThread
{
	Q_OBJECT
	
public:
	//AbstractInputHandler(shared_data *sd, QObject *parent = 0);
	virtual ~AbstractInputHandler();
	virtual void setId(QString i) {_id = i;};
	virtual QString id() const {return _id;};
	virtual void setOutputs(QVector<OutEvent> o);
	virtual QVector<OutEvent> getOutputs() const {return outputs;};
	virtual int addInputCode(__u16 in);
	virtual int addInputCode(__u16 in, OutEvent def);
	void sendMouseEvent(VEvent *e, int num = 1);
	void sendKbdEvent(VEvent *e, int num = 1);
	void sendTextEvent(TEvent *t);
	QString getId() const {return _id;};
	int getNumInputs() const {return inputs.size();};
	int getNumOutputs() const {return outputs.size();};
	
protected:
	QString _id;
	shared_data *sd;
	QVector<__u16> inputs;
	QVector<OutEvent> outputs;
};

#endif // ABSTRACTINPUTHANDLER_H
