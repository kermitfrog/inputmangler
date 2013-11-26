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


#include "abstractinputhandler.h"
#include "inputmangler.h"
#include <QDebug>
#include <QTest>

// AbstractInputHandler::AbstractInputHandler(shared_data *sd, QObject *parent)
// {
// 
// }

AbstractInputHandler::~AbstractInputHandler()
{
}

int AbstractInputHandler::addInputCode(__u16 in)
{
	inputs.append(in);
	outputs.append(OutEvent(in));
	return inputs.size();
}


int AbstractInputHandler::addInputCode(__u16 in, OutEvent def)
{
	inputs.append(in);
	outputs.append(def);
	return inputs.size();
}


void AbstractInputHandler::setOutputs(QVector< OutEvent > o)
{
	if (o.size() != inputs.size())
	{
		qDebug() << "setOutputs: in|out sizes do not match ";
		for (int i = 0; i < inputs.size(); i++)
			qDebug() << "inputs[" << i << "].code = " << inputs.at(i);
		for (int i = 0; i < o.size(); i++)
			qDebug() << "outputs[" << i << "].code = " << o.at(i).code();
	}	
	outputs = o;
}

void AbstractInputHandler::sendTextEvent(TEvent* t)
{
	VEvent e[NUM_MOD*2+2];
	QVector<__u16> m = t->modifiers;
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
	e[k].code = t->code;
	e[k].value = 1;
	e[k+1].type = EV_KEY;
	e[k+1].code = t->code;
	e[k+1].value = 0;
// 	for (int i = 0; i < m.size() * 2 + 2; i++)
// 		qDebug() << e[i].type << " " << e[i].code << " " << e[i].value << " L= " << m.size()*2+2;
	sendKbdEvent(e, m.size() * 2 + 2);
	usleep(5000); // wait x * 0.000001 seconds
}

TEvent::TEvent(__s32 c, bool s, bool a, bool C)
{
	if (s)
		modifiers.append(KEY_LEFTSHIFT);
	if (a)
		modifiers.append(KEY_RIGHTALT);
	if (C)
		modifiers.append(KEY_LEFTCTRL);
	code = c;
}

//FIXME: should be inline, but then code does not link -> WTF???
void AbstractInputHandler::sendMouseEvent(VEvent* e, int num) 
{
#ifdef DEBUGME
// 	qDebug() << "Mouse sending: " 
// 	<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(sd->fd_mouse, e, num*sizeof(VEvent));
}

inline void AbstractInputHandler::sendKbdEvent(VEvent* e, int num)
{
	
#ifdef DEBUGME
	qDebug() << "Kbd sending: " 
	<< QTest::toHexRepresentation(reinterpret_cast<char*>(e), sizeof(VEvent)*(num));
#endif
	write(sd->fd_kbd, e, num*sizeof(VEvent));
}



