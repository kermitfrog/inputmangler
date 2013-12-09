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
	// set up key definitions
	setUpKeymaps(); // keys as well as char mappings and multi-char commands (nethandler)
	
	// open X Display. we need it only to get the window class
	display = XOpenDisplay(NULL);
	if (!display)
		qFatal("connection to X Server failed");
	readConf();
}

// Reads the Config and actually starts the handler threads, too
bool InputMangler::readConf()
{
	// set up shared data
	sd = new shared_data;
	sd->fd_kbd = open("/dev/virtual_kbd", O_WRONLY|O_APPEND);
	sd->fd_mouse = open("/dev/virtual_mouse", O_WRONLY|O_APPEND);
#ifdef DEBUGME	
	qDebug() << "kbd: " << sd->fd_kbd << ", mouse: " << sd->fd_mouse;
#endif
	
	//parsing config
	QString confPath;
	if (QCoreApplication::arguments().count() < 2)
		confPath = QDir::homePath() + "/.config/inputMangler/config.xml";
	else
		confPath = QCoreApplication::arguments().at(1);
	QFile f(confPath);
	QDomDocument conf;
	QDomNodeList nodes;
	conf.setContent(&f);
	
	// get a list of available devices from /proc/bus/input/devices
	QList<idevs> availableDevices = parseInputDevices();
	/// Devices
	nodes = conf.elementsByTagName("device");
	// for every configured <device...>
	for (int i = 0; i < nodes.length(); i++)
	{
		/*
		 * create an idevs structure for the configured device
		 * vendor and product are set to match the devices in /proc/bus/...
		 * id is the config-id 
		 */
		idevs d;
		d.vendor  = nodes.at(i).attributes().namedItem("vendor").nodeValue();
		d.product = nodes.at(i).attributes().namedItem("product").nodeValue();
		d.id      = nodes.at(i).attributes().namedItem("id").nodeValue();
		
		// create a devhandler for every device that matches vendor and product
		// of the configured device,
		while (availableDevices.count(d))
		{
			int idx = availableDevices.indexOf(d);
			// copy information obtained from /proc/bus/input/devices to complete
			// the data in the idevs object used to construct the DevHandler
			d.event = availableDevices.at(idx).event;
			d.mouse = availableDevices.at(idx).mouse;
			handlers.append(new DevHandler(d, sd));
			availableDevices.removeAt(idx);
			
			/*
			 * read the <signal> entries.
			 * [key] will be the input event, that will be transformed
			 * [default] will be the current output device, this is 
			 * transformed to. If no [default] is set, the current output will
			 * be the same as the input.
			 * For DevHandlers with window specific settings, the current output
			 * becomes the default output when the TransformationStructure is
			 * constructed, otherwise it won't ever change anyway...
			 */
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
	
	/// netfhandler
	nodes = conf.elementsByTagName("net");
	NetHandler *n;
	for (int i = 0; i < nodes.length(); i++)
	{
	n = new NetHandler(	sd,
						nodes.at(i).attributes().namedItem("addr").nodeValue(),
						nodes.at(i).attributes().namedItem("port").nodeValue().toInt() );
	handlers.append(n);
	}
	
	/// check if there is a reason to continue at all...
	if (handlers.count() == 0)
	{
		qDebug() << "no input Handlers loaded!";
		exit(1);
	}
	
	/// Window-specific settings
	QStringList ids;
	foreach(AbstractInputHandler *a, handlers)
	{
		if (!a->hasWindowSpecificSettings)
			continue;
		// make current outputs the default
		// if nessecary, a new TransformationStructure is created on demand by QMap,
		QVector<OutEvent> o = a->getOutputs();
		wsets[a->id()].def = o;
		ids.append(a->id());
	}
	ids.removeDuplicates();
	
	nodes = conf.elementsByTagName("window");
	// for each <window>
	for (int i = 0; i < nodes.length(); i++) 
	{
		QDomElement e = nodes.at(i).toElement();
		foreach(QString id, ids) // where id = M|F|? -> for each id
		{
			/*
			 * if the <window> has no default settings for the id,
			 * it is still possible, there are are settings for 
			 * special titles.
			 * We need to check that to decide if a WindowSettings
			 * object needs to be created.
			 */
			bool hasNoIdInWindowButTitlesWithId = false;
			QDomNodeList titles = e.elementsByTagName("title");
			if (!e.hasAttribute(id))
			{
				for (int k = 0; k < titles.length(); k++)
				{
					QDomElement t = titles.at(k).toElement();
					if (t.hasAttribute(id))
					{
						hasNoIdInWindowButTitlesWithId = true;
						break;
					}
				}
				if (!hasNoIdInWindowButTitlesWithId)
					continue;
			}
				
			// create WindowSettings
			WindowSettings *w = wsets[id].window(e.attribute("class"), true);
			/*
			 * set default outputs for that window class.
			 * in the special case mentioned above, the window default output
			 * events are same as the id default.
			 */
			if (hasNoIdInWindowButTitlesWithId)
				w->def = wsets[id].def;
			else
			{
				QStringList l = e.attribute(id).split(",");
				foreach(QString s, l)
					w->def.append(OutEvent(s));
			}

			// for each <title> inside the current <window>
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
	
	// prepare and actualy start threads. also roll a sanity check
	sd->terminating = false;
	
	activeWindowTitleChanged("");
	bool sane;
	foreach (AbstractInputHandler *h, handlers)
	{
		sane = false;
		if (h->getId() == "" && h->getNumInputs() == h->getNumOutputs() && h->getNumInputs())
			sane = true;
		else if(!h->hasWindowSpecificSettings )
			sane = true;
		else 
			sane = wsets[h->id()].sanityCheck(h->getNumInputs(), h->getId());
		
		if (sane)
			h->start();
	}
	
	f.close();
}


/* 
 * parses /proc/bus/input/devices for relevant information, where relevant is:
 * I: Vendor=1395 Product=0020
 * H: Handlers=kbd event9     <-- kbd or mouse? which event file in /dev/input/ is it?
 */
QList< idevs > InputMangler::parseInputDevices()
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

/*
 * this is called when the active window changes
 * so far it gets the new window *title* as parameter
 * this will probably change as soon as the kwin-scripting api
 * offers a way to get the window class
 */
void InputMangler::activeWindowChanged(QString w)
{
	if (sd->terminating)
		return;
	Window active;
	int revert;
	XClassHint window_class;
	
	if (XGetInputFocus(display, &active, &revert) == BadWindow)
		qDebug() << "BadWindow";
	if (active == None || active == PointerRoot)
		return;
	
	if (!XGetClassHint(display, active, &window_class)) {
		qDebug() << "Could not get Window Class, where title is " << w;
		//return;
		//HACK:: this problem seems to occur only with Opera, so..
		wm_class = "Opera";
		
	} else {
		
		wm_class = QString(window_class.res_class);
	//	wm_class = QString(window_class.res_name);
		
		
		XFree(window_class.res_class);
		XFree(window_class.res_name);
	}
		wm_title = w;
		
	
// 	qDebug() << "wm_class = " << wm_class << "; wm_title = " << wm_title;// << "wm_class2: " << window_class.res_class;
	
	//update handlers
// 	qDebug() << "update handlers: in";
	foreach (AbstractInputHandler *a, handlers)
		if (a->hasWindowSpecificSettings)
			a->setOutputs(wsets[a->id()].getOutputs(wm_class, wm_title));
// 	qDebug() << "update handlers: out";
	
}

/*
 * this is called when the title of the active window changes
 * for now it just calls activeWindowChanged
 * this will probably change as soon as the kwin-scripting api
 * offers a way to get the window class
 */
void InputMangler::activeWindowTitleChanged(QString w)
{
	activeWindowChanged(w);
}

/*
 * end threads and close all devices
 */
void InputMangler::cleanUp()
{
	sd->terminating = true;
	qDebug() << "waiting for Threads to finish";
	foreach (AbstractInputHandler *h, handlers)
		h->wait(4000);
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
	XFree(display); // is that the right way to close the connection with X?
}

// Constructs an output event from a config string
OutEvent::OutEvent(QString s)
{
#ifdef DEBUGME
	initString = s;
#endif
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
		if(l[1].contains(""))
			modifiers = modifiers | MOD_MACRO;*/
	}
}

// get the window structure for window w
// if create is true: create a new window, when none is found
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

// get output events for a given window and window title
QVector< OutEvent > TransformationStructure::getOutputs(QString window_class, QString window_name)
{
	//qDebug() << "getOutputs(" << c << ", " << n << ")";
	WindowSettings *w = window(window_class);
	if (w == NULL)
		return def;
	//qDebug() << "Window found with " << w->titles.size() << "titles";
	int idx;
	for (int i = 0; i < w->titles.size(); i++)
	{
		//qDebug() << "Title: " << w->titles.at(i)->pattern();
		QRegularExpressionMatch m = w->titles.at(i)->match(window_name);
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

// check a TransformationStructure for configuration errors
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
#ifdef DEBUGME 
		qDebug() << "Settings found for Window = \"" << classes.key(w) << "\"";
#endif
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
#ifdef DEBUGME
			qDebug() << "  with Pattern: \"" << w->titles[i]->pattern() << "\"";
#endif
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

OutEvent::OutEvent(__s32 code, bool shift, bool alt, bool ctrl)
{
	if (shift)
		modifiers.append(KEY_LEFTSHIFT);
	if (alt)
		modifiers.append(KEY_RIGHTALT);
	if (ctrl)
		modifiers.append(KEY_LEFTCTRL);
	this->keycode = code;
}