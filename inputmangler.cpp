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
#include "nethandler.h"
#include <fcntl.h>
#include <unistd.h>
#include <QtXml>
#include <signal.h>
#include <X11/Xlib.h>
#include "keydefs.h"


InputMangler::InputMangler()
{
	setUpKeymap();
	setUpCMap();
	setUpSMap();
	
	
	display = XOpenDisplay(NULL);
	if (!display)
		qFatal("connection to X Server failed");
	readConf();
}

bool InputMangler::readConf()
{
	sd = new shared_data;
	sd->fd_kbd = open("/dev/virtual_kbd", O_WRONLY|O_APPEND);
	sd->fd_mouse = open("/dev/virtual_mouse", O_WRONLY|O_APPEND);
	
	//parsing config
	QFile f(QDir::homePath() + "/.config/inputMangler/config.xml");
	QDomDocument conf;
	QDomNodeList nodes;
	conf.setContent(&f);
	
	QList<idevs> availableDevices = parseInputDevices();
	/// Devices
	nodes = conf.elementsByTagName("device");
	for (int i = 0; i < nodes.length(); i++)
	{
		idevs d;
		d.vendor  = nodes.at(i).attributes().namedItem("vendor").nodeValue();
		d.product = nodes.at(i).attributes().namedItem("product").nodeValue();
		d.id      = nodes.at(i).attributes().namedItem("id").nodeValue();
		
		
		while (availableDevices.count(d))
		{
			int idx = availableDevices.indexOf(d);
			d.event = availableDevices.at(idx).event;
			d.mouse = availableDevices.at(idx).mouse;
			handlers.append(new DevHandler(d, sd));
			availableDevices.removeAt(idx);
				QDomNodeList codes = nodes.at(i).toElement().elementsByTagName("signal");
				for (int j = 0; j < codes.length(); j++)
				{
					QString key = codes.at(j).attributes().namedItem("key").nodeValue();
					QString def = codes.at(j).attributes().namedItem("default").nodeValue();
					if (def == "")
						handlers.last()->addInputCode(keymap[key]);
					else
						handlers.last()->addInputCode(keymap[key], OutEvent(def));
				}
		}
	}
	
	/// TODO: initialise netstuff here
	nodes = conf.elementsByTagName("net");
	NetHandler *n;
	for (int i = 0; i < nodes.length(); i++)
	{
	n = new NetHandler(	sd,
						nodes.at(i).attributes().namedItem("addr").nodeValue(),
						nodes.at(i).attributes().namedItem("port").nodeValue().toInt() );
	handlers.append(n);
	}
	
	
	
	
	if (handlers.count() == 0)
	{
		qDebug() << "no input Handlers loaded!";
		exit(1);
	}
	
	/// Window-specific settings
	QStringList ids;
	foreach(AbstractInputHandler *a, handlers)
	{
		QVector<OutEvent> o = a->getOutputs();
		wsets[a->id()].def = o;
		if (a->id() != "")
			ids.append(a->id());
	}
	ids.removeDuplicates();
	
	nodes = conf.elementsByTagName("window");
	for (int i = 0; i < nodes.length(); i++) //for each window
	{
		QDomElement e = nodes.at(i).toElement();
		foreach(QString id, ids) // where id = M|F|? -> for each id
		{
			if (!e.hasAttribute(id)) //FIXME: aborts if there are title-tags, but no F|M|?
				continue;
			QStringList l = e.attribute(id).split(",");
			WindowSettings *w = wsets[id].window(e.attribute("class"), true);
			foreach(QString s, l)
				w->def.append(OutEvent(s));
			
			//title
			QDomNodeList titles = e.elementsByTagName("title");
			for (int k = 0; k < titles.length(); k++)
			{
				QDomElement t = titles.at(k).toElement();
				if (!t.hasAttribute(id))
					continue;
				w->titles.append(new QRegularExpression(QString("^") + t.attribute("regex") + "$"));
				QStringList l = t.attribute(id).split(",");
				QVector<OutEvent> o;
				foreach(QString s, l)
					o.append(OutEvent(s));
				w->events.append(o);
			}
		}	
	}
	sd->terminating = false;
	
	activeWindowTitleChanged("");
	bool sane = false;
	foreach (AbstractInputHandler *h, handlers)
	{
		if(h->getId() == "___NET")
			sane = true;
		else if (h->getId() == "" && h->getNumInputs() == h->getNumOutputs() && h->getNumInputs())
			sane = true;
		else 
			sane = wsets[h->id()].sanityCheck(h->getNumInputs(), h->getId());
		
		if (sane)
			h->start();
	}
	
	f.close();

}


/* parses /proc/bus/input/devices for relevant information*/
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
			i.mouse = (tmp.indexOf(QRegExp("mouse.*")) != -1);
			++li;
			break;
		}
		l.append(i);
	}
	return l;
}

void InputMangler::activeWindowChanged(QString w)
{
	if (sd->terminating)
		return;
	//Window active;
	//int revert;
	Window active;
	int revert;
	XClassHint window_class;
	
	
	XGetInputFocus(display, &active, &revert);
	if (!active)
		qFatal("could not get Active Window from X");
	
	if (!XGetClassHint(display, active, &window_class)) {
		qDebug() << "Could not get Window Class, mhere title is " << w;
		return;
	}
		
//	wm_class = QString(window_class.res_class);
 	wm_class = QString(window_class.res_name);
	wm_title = w;
	
	//qDebug() << "wm_class = " << wm_class << "; wm_title = " << wm_title;// << "wm_class2: " << window_class.res_class;
	
	XFree(window_class.res_class);
	XFree(window_class.res_name);
	//update handlers
// 	qDebug() << "update handlers: in";
	foreach (AbstractInputHandler *a, handlers)
		a->setOutputs(wsets[a->id()].getOutputs(wm_class, wm_title));
// 	qDebug() << "update handlers: out";
	
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
		h->wait(5000);
	close(sd->fd_kbd);
	close(sd->fd_mouse);
	foreach (AbstractInputHandler *h, handlers)
		delete h;
	delete sd;
	handlers.clear();
	wsets.clear();
}


InputMangler::~InputMangler()
{
	XFree(display); // is that right?
}

OutEvent::OutEvent(QString s)
{
	keycode = 0;
	QStringList l = s.split("+");
	if (l.empty())
		return;
	keycode = keymap[l[0]];
	if (l.length() > 1)
	{
		if(l[1].contains("S"))
			modifiers.append(keymap["S"]);
		if(l[1].contains("A"))
			modifiers.append(keymap["A"]);
		if(l[1].contains("C"))
			modifiers.append(keymap["C"]);
		if(l[1].contains("M"))
			modifiers.append(keymap["M"]);
		if(l[1].contains("G") || l[1].contains("3"))
			modifiers.append(keymap["RIGHTALT"]);
		/*if(l[1].contains("~"))
			modifiers = modifiers | MOD_REPEAT;
		if(l[1].contains("@"))
			modifiers = modifiers | MOD_MACRO;*/
	}
}

WindowSettings* TransformationStructure::window(QString w, bool create)
{
	if (create)
	{
		if(!classes.contains(w))
			classes.insert(w, new WindowSettings());
		return classes.value(w);
	}
	if (!classes.contains(w))
		return NULL;
	return classes.value(w);
}


QVector< OutEvent > TransformationStructure::getOutputs(QString c, QString n)
{
	//qDebug() << "getOutputs(" << c << ", " << n << ")";
	WindowSettings *w = window(c);
	if (w == NULL)
		return def;
	//qDebug() << "Window found with " << w->titles.size() << "titles";
	int idx;
	for (int i = 0; i < w->titles.size(); i++)
	{
		//qDebug() << "Title: " << w->titles.at(i)->pattern();
		QRegularExpressionMatch m = w->titles.at(i)->match(n);
		if(m.hasMatch())
			return w->events.at(i);
	}
	return w->def;
}

WindowSettings::~WindowSettings()
{
	foreach (QRegularExpression* r, titles)
		delete r;
}

TransformationStructure::~TransformationStructure()
{
	foreach (WindowSettings * w, classes)
		delete w;
}

void InputMangler::reReadConfig()
{
	cleanUp();
	readConf();
}

bool TransformationStructure::sanityCheck(int s, QString id)
{
	bool result = true;
	qDebug() << "checking " << id << " with size " << s;
	if (this->def.size() != s)
	{
		qDebug() << "TransformationStructure.def is " << this->def.size();
		result = false;
	}
	QList<WindowSettings*> wlist = classes.values();
	foreach (WindowSettings *w, wlist)
	{
		if (w->def.size() != s)
		{
			qDebug() << "WindowSettings.def for " << classes.key(w) << " is " << def.size();
			result = false;
		}
		if (w->events.size() != w->titles.size())
		{
			qDebug() << "WindowSettings.titles = " << w->titles.size() 
					 << " events = " << w->events.size() << " for " << classes.key(w);
			result = false;
		}
		for(int i = 0; i < w->events.size(); i++)
		{
			if (w->events.at(i).size() != s)
			{
				qDebug() << "WindowSettings for " << classes.key(w) << ":" 
						 << "Regex = \"" << w->titles.at(i)->pattern() << "\", size = " 
						 << w->events.at(i).size();
				result = false;
			}
		}
	}
	if (!result)
		qDebug() << id << " failed SanityCheck!!!";
	return result;
}

