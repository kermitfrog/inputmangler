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


#include "imdbusinterface.h"
#include "inputmangler.h"
#include "devhandler.h"
#include <fcntl.h>
#include <unistd.h>
#include <QtXml>
#include <signal.h>
#include <X11/Xlib.h>
// #include "keydefs.h"


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
	
	display = XOpenDisplay(NULL);
	if (!display)
		qFatal("connection to X Server failed");

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
void InputMangler::activeWindowChanged(QString w)
{
	Window active;
	int revert;
	XClassHint window_class;
	
	
	XGetInputFocus(display, &active, &revert);
	if (!active)
		qFatal("could not get Active Window from X");
	
	XGetClassHint(display, active, &window_class);
	wm_class = QString(window_class.res_class);
	
	
	
	wm_title = w;//getThatStupidWindowTitleFromX(&active);
	
	
	//XFree(window_name);
}

QString InputMangler::getThatStupidWindowTitleFromX(Window *window)
{
/*	Atom netWmName;
	Atom utf8;
	Atom actType;
	int actFormat;
	unsigned long nItems, bytes;
	unsigned char **data;

	netWmName = XInternAtom(display, "_NET_WM_NAME", False);
	utf8 = XInternAtom(display, "UTF8_STRING", False);

	XGetWindowProperty(display, *window, netWmName, 0, 0x77777777, False, utf8, &actType,  &actFormat, &nItems, &bytes, (unsigned char **) &data);

	qDebug() << "argh: " << bytes;


	
	XTextProperty window_name;
	XGetWMName(display, *window, &window_name);
	//char *** text;
	char** text = NULL;
	int count;
	qDebug() << "format = " << window_name.format << ", nitems = "<< window_name.nitems;
	if (window_name.nitems < 1)
	{
		qDebug() << "Stupid XTextProperty: nitems = " << window_name.nitems << " wtf???";
		return "";
	}
	switch (window_name.format)
	{
		case 8:
			XmbTextPropertyToTextList(display, &window_name, &text, &count);
			break;
		case 16:
			
		case 32:
			
		default:
			qDebug() << "Stupid XTextProperty: format != 8|16|32 ... what now?";
			return "Error: Name in unsupported Format";
	}
	if (count != 1)
	{
		qDebug() << "Stupid XTextProperty: count = " << count << " wtf???";
		return "";
	}
	QString r(text[0]);
	if (r == "")
		qDebug() << "Stupid XTextProperty: empty result... great... >.<";
	return r;
	*/
}


void InputMangler::activeWindowTitleChanged(QString w)
{
	activeWindowChanged(w);
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

