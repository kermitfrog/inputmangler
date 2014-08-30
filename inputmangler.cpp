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

#include <unistd.h>
#include <QtXml>
#include <signal.h>
#include "imdbusinterface.h"
#include "inputmangler.h"
#include "handlers.h"
#include "keydefs.h"


InputMangler::InputMangler()
{
	registerHandlers();
	readConf();
}

/*!
 * @brief  Reads the Config and actually starts the handler threads, too
 */
bool InputMangler::readConf()
{
	// open output devices
	OutEvent::generalSetup();
	AbstractInputHandler::generalSetup();
	//parse config
	QString confPath;
	if (QCoreApplication::arguments().count() < 2)
		confPath = QDir::homePath() + "/.config/inputMangler/config.xml";
	else
		confPath = QCoreApplication::arguments().at(1);
	QFile f(confPath);
	QDomDocument conf;
	QDomNodeList nodes;
	conf.setContent(&f);
	
	// set up key definitions as well as char mappings and multi-char commands (nethandler)
	{
		QDomElement e = conf.elementsByTagName("mapfiles").at(0).toElement();
		QString k, c, a;
		k = e.attribute("keymap");
		c = e.attribute("charmap");
		a = e.attribute("axismap");
 		qDebug() << "using keymap = " << k << ", charmap = " << c << ", axismap = " << a;
		setUpKeymaps(k, c, a);
	}
	// all handler-specific configuration is read by the registered parsing functions
	foreach (QString nodeName, AbstractInputHandler::parseMap.keys())
	{
		nodes = conf.elementsByTagName(nodeName);
		handlers.append(AbstractInputHandler::parseMap[nodeName](nodes));
		// FIXME: write a proper interface for doing this or something
		if (nodeName == "xwatcher")
		{
			XWatcher *xwatcher = static_cast<XWatcher*>(handlers.last());
			connect(xwatcher, SIGNAL(windowChanged(QString,QString)),
					SLOT(activeWindowChanged(QString,QString)));
			connect(xwatcher, SIGNAL(windowTitleChanged(QString)),
					SLOT(activeWindowTitleChanged(QString)));
		}
	}
	foreach (AbstractInputHandler* handler, handlers)
	{
		if (handler->id() != "")
			handlersById.insert(handler->id(), handler);
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
		if (!a->hasWindowSpecificSettings())
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
		QDomElement element = nodes.at(i).toElement(); // representing a <window> structure
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
			QDomNodeList titles = element.elementsByTagName("title");
			if (!hasSettingsForId(id, element))
			{
				for (int k = 0; k < titles.length(); k++)
				{
					QDomElement t = titles.at(k).toElement(); // representing a <title> structure
					if (hasSettingsForId(id, t))
					{
						hasNoIdInWindowButTitlesWithId = true;
						break;
					}
				}
				if (!hasNoIdInWindowButTitlesWithId)
					continue;
			}
			
			// create WindowSettings
			WindowSettings *windowSetting = wsets[id].window(element.attribute("class"), true);
			/*
			 * set default outputs for that window class.
			 * in the special case mentioned above, the window default output
			 * events are same as the id default.
			 */
			if (hasNoIdInWindowButTitlesWithId)
				windowSetting->def = wsets[id].def;
			else
				windowSetting->def = parseOutputs(id, element, wsets[id].def);

			// for each <title> inside the current <window>
			for (int k = 0; k < titles.length(); k++)
			{
				QDomElement t = titles.at(k).toElement(); // <title>
				if (!t.hasAttribute(id))
					continue;
				windowSetting->titles.append(new QRegularExpression(QString("^") + t.attribute("regex") + "$"));
				windowSetting->events.append(parseOutputs(id, t, windowSetting->def));
			}
		}
	}
	
	// Prepare and actualy start threads. Also roll a sanity check.
	AbstractInputHandler::sd.terminating = false;
	
	// Make sure outputs and window & class name are set to something...
	// May be unneccessary, but better safe than sorry.
	activeWindowTitleChanged("");
	
	bool sane;
	foreach (AbstractInputHandler *h, handlers)
	{
		sane = false;
		if (h->id() == "" && h->getNumInputs() == h->getNumOutputs() && h->getNumInputs())
			sane = true;
		else if(!h->hasWindowSpecificSettings() )
			sane = true;
		else 
			sane = wsets[h->id()].sanityCheck(h->getNumInputs(), h->id());
		
		if (sane)
			h->start();
	}
	
	f.close();
}

/*!
 * @brief Check if element has any output definition for TransformationStructure with id
 * @param id Id of a TransformationStructure
 * @param element XML element, typicaly <window> or <title>
 */
bool InputMangler::hasSettingsForId(QString id, QDomElement element)
{
	// short version
	if (element.hasAttribute(id))
		return true;
	// long version
	QDomNodeList nodes = element.elementsByTagName("long");
	for (int i = 0; i < nodes.length(); i++)
		if (nodes.at(i).attributes().namedItem("id").nodeValue() == id)
			return true;
	return false;
}

/*!
 * @brief Parse output definition for TransformationStructure with id. 
 * Returns def when nothing is found.
 * @param id Id of a TransformationStructure
 * @param element XML element, typicaly <window> or <title>
 * @param def Default value. Returned when no output definition is found. Result of
 * <long> description is based on this.
 */
QVector< OutEvent > InputMangler::parseOutputs(QString id, QDomElement element, QVector< OutEvent > def)
{
	// short version
	if (element.hasAttribute(id))
	{
		def.clear();
		QStringList l = element.attributes().namedItem(id).nodeValue().split(",");
		foreach(QString s, l)
			def.append(OutEvent(s));
		return def;
	}
	// long version
	QDomNodeList nodes = element.elementsByTagName("long");
	for(int i =  0; i < nodes.length(); i++)
	{
		if (nodes.at(i).attributes().namedItem("id").nodeValue() != id)
			continue;
		// valid seperators are '\n' and ','
		QStringList lines = nodes.at(i).toElement().text().split(QRegExp("(\n|,)"));
		foreach (QString s, lines)
		{
			// we want allow a dual use of '=', e.g. <long> ==c, r== </long>
			s = s.trimmed();
			int pos = s.indexOf('=', 1);
			QString left = s.left(pos);
			QString right = s.mid(pos + 1);
			if (left.isEmpty() && right.isEmpty())
				return def;
			
			// find the right input index
			QMap<QString,AbstractInputHandler*>::iterator handler = handlersById.find(id);
			while(handler != handlersById.end() && handler.key() == id)
			{
				int idx = handler.value()->inputIndex(left);
				if (idx >= 0)
				{
					def[idx] = OutEvent(right);
					break;
				}
				++handler;
			}
		}
	}
	return def;
}

/*!
 * @brief this is called when the active window changes.
 * It updates the Outputs of all handlers.
 * @param wclass The new window class.
 * @param title The new window title.
 */
void InputMangler::activeWindowChanged(QString wclass, QString title)
{
	if (AbstractInputHandler::sd.terminating)
		return;
		
	wm_class = wclass;
	wm_title = title;
	
	foreach (AbstractInputHandler *a, handlers)
		if (a->hasWindowSpecificSettings())
			a->setOutputs(wsets[a->id()].getOutputs(wm_class, wm_title));
	
}

/*!
 * @brief This is called when the title of the active window changes.
 * It just calls activeWindowChanged() with the current class
 * and the new title
 */
void InputMangler::activeWindowTitleChanged(QString title)
{
//	qDebug() << "InputMangler::activeWindowTitleChanged(" << title << ")";
	activeWindowChanged(wm_class, title);
}

/*!
 * @brief End threads and close all devices.
 */
void InputMangler::cleanUp()
{
	AbstractInputHandler::sd.terminating = true;
	qDebug() << "waiting for Threads to finish";
	foreach (AbstractInputHandler *h, handlers)
		h->wait(4000);
	close(OutEvent::fd_kbd);
	close(OutEvent::fd_mouse);
	foreach (AbstractInputHandler *h, handlers)
		delete h;
	handlers.clear();
	wsets.clear();
}

/*!
 * @brief Print the current window class and title to console.
 * Called by sending a USR1 signal.
 * TODO: make it work as a dbus call, returning the string.
 */
void InputMangler::printWinInfo()
{
	qDebug() << "Class = " << wm_class << ", Title = " << wm_title;
}

/*!
 * @brief Print the current window specific config to console.
 * Called by sending a USR2 signal.
 * TODO: make it work as a dbus call, returning the string.
 */
void InputMangler::printConfig()
{
	QStringList checkedIds;
	foreach (AbstractInputHandler *h, handlers)
	{
		if(!h->hasWindowSpecificSettings() || h->id() == "")
			continue;
		if (checkedIds.contains(h->id()))
			continue;
		checkedIds.append(h->id());
		wsets[h->id()].sanityCheck(h->getNumInputs(), h->id(), true);
	}
}

InputMangler::~InputMangler()
{
}

/*!
 * @brief Clean up and reread the configuration. Basicaly a restart without a restart.
 * Called by sending a HUP signal.
 * TODO: make it work as a dbus call.
 */
void InputMangler::reReadConfig()
{
	cleanUp();
	readConf();
}

