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


#include "devhandler.h"
#include "inputmangler.h"
#include <poll.h>       //poll
#include <QDataStream>
#include <QDebug>
#include <linux/input.h>
#include <unistd.h>

void DevHandler::run()
{
	if(!d.open(QIODevice::ReadOnly|QIODevice::Unbuffered));
	{
		qDebug() << "?could (not?) open " << d.fileName();
		//return;
	}
	struct pollfd p;
	p.fd = d.handle();
	p.events = POLLIN;
	p.revents = POLLIN;
	
	int ret, n; int testval;
	QDataStream data(&d);

 	//char buf[80];
	input_event buf[4];

	while (1)
	{
		ret = poll (&p, 1, 1500);
		if(sd->terminating)
		{
			qDebug() << "terminating" << "btw: sizeof input_event is " << sizeof(input_event);
			break;
		}
		if(ret)
		{
			//n = d.read((char*)buf, 95);
			n = read(d.handle(), buf, 96);
			for (int i = 0; i < n; i++)
				//qDebug("%d:%d ", n, buf[n]);
				qDebug("time: %d, type: %d, code: %d, value: %d", buf[n].time, buf[n].type, buf[n].code, buf[n].value );
			qDebug() << id << ":" << n;
		}
	}
	
}


DevHandler::DevHandler(idevs i, shared_data *sd)
{
	this->sd = sd;
	id = i.id;
	qDebug() << "id is " << i.id;
	d.setFileName(QString("/dev/input/") + i.event);

}

DevHandler::~DevHandler()
{

}

