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
#include <unistd.h>


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
	return inputs.size();
}


int AbstractInputHandler::addInputCode(__u16 in, OutEvent def)
{
	inputs.append(in);
	outputs.append(def);
	return inputs.size();
}

void AbstractInputHandler::sendKbdEvent(VEvent* e, int num)
{
	write(sd->fd_kbd, e, num*sizeof(VEvent));
}

void AbstractInputHandler::sendMouseEvent(VEvent* e, int num)
{
	write(sd->fd_mouse, e, num*sizeof(VEvent));
}

void AbstractInputHandler::setOutputs(QVector< OutEvent > o)
{
	outputs = o;
	if (o.size() > 0)
		qDebug() << "keycode of " << id() << " is " << outputs[0].keycode;
}


