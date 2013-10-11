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


#include "inputmangler.h"
#include "devhandler.h"
#include <fcntl.h>
#include <unistd.h>
#include <QtXml>
#include <signal.h>



InputMangler::InputMangler()
{
	sd = new shared_data;
	sd->fd_kbd = open("/dev/virtual_kbd", O_WRONLY|O_APPEND);
	sd->fd_mouse = open("/dev/virtual_mouse", O_WRONLY|O_APPEND);
	sd->terminating = false;
	
	
	QFile f("/home/arek/projects/inputMangler/config.xml");
	QDomDocument conf;
	conf.setContent(&f);
	
	QList<idevs> availableDevices = parseInputDevices();
	QDomNodeList nodes = conf.elementsByTagName("device");
	for (int i = 0; i < nodes.length(); i++)
	{
		idevs d;
		d.vendor  = nodes.at(i).attributes().namedItem("vendor").nodeValue();
		d.product = nodes.at(i).attributes().namedItem("product").nodeValue();
		d.id      = nodes.at(i).attributes().namedItem("id").nodeValue();
		qDebug() << "id for " << i << " is " << d.id;
		while (availableDevices.count(d))
		{
			int idx = availableDevices.indexOf(d);
			d.event = availableDevices.at(idx).event;
			handlers.append(new DevHandler(d, sd));
			availableDevices.removeAt(idx);
		}
	}
	if (handlers.count() == 0)
	{
		qDebug() << "no input Handlers loaded!";
		exit(1);
	}
	
	foreach (AbstractInputHandler *h, handlers)
		h->start();
	
	//QCoreApplication::quit();

}

QList< idevs > InputMangler::parseInputDevices()
{
	QFile d("/proc/bus/input/devices");
	
	if(!d.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not open " << d.fileName();
		return QList<idevs>();
	}
	QTextStream t(&d);
	QStringList devs = t.readAll().split("\n");
	QList<idevs> l;
	idevs i;
	QStringList tmp;
	int idx;
	
	QList<QString>::iterator li = devs.begin();
	while (li != devs.end())
	{
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
			if(!(*li).startsWith("H"))
			{
				++li;
				continue;
			}
			tmp = (*li).split(QRegExp("[\\s=]"));
			idx = tmp.indexOf(QRegExp("event.*"));
			i.event = tmp.at(idx);
			++li;
			break;
		}
		l.append(i);
	}
	return l;
}

void InputMangler::cleanUp()
{
	sd->terminating = true;
	qDebug() << "waiting for Threads to finish";
	foreach (AbstractInputHandler *h, handlers)
		h->wait(2500);
	qDebug() << "waited long enough";
	
}


InputMangler::~InputMangler()
{
	//TODO wait for threads to finish
	close(sd->fd_kbd);
	close(sd->fd_mouse);
	foreach (AbstractInputHandler *h, handlers)
		delete h;
}

