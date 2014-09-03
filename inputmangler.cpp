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
#include <QXmlStreamReader>
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
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		qDebug() << "could not open configuration File for reading: " << confPath;
	QXmlStreamReader conf(&f);
	
	// set up key definitions as well as char mappings and multi-char commands (nethandler)
	// this must be done *before* anything else is parsed.
	QString mapfileK, mapfileC, mapfileA;
	while(!conf.atEnd() && !conf.hasError())
	{
		if(conf.isStartElement())
		{
			if (conf.name() == "mapfiles")
			{
				QXmlStreamAttributes mapfiles = conf.attributes();
				if(mapfiles.hasAttribute("keymap"))
					mapfileK = mapfiles.value("keymap").toString();
				if(mapfiles.hasAttribute("charmap"))
					mapfileC = mapfiles.value("charmap").toString();
				if(mapfiles.hasAttribute("axismap"))
					mapfileA = mapfiles.value("axismap").toString();
			}
		}
		conf.readNextStartElement();
	}
	qDebug() << "using keymap = " << mapfileK << ", charmap = " << mapfileC << ", axismap = " << mapfileA;
	setUpKeymaps(mapfileK, mapfileC, mapfileA);
	
	// on the second pass, configure all the handlers
	f.seek(0);
	conf.setDevice(&f);
	conf.readNextStartElement();
	while(!conf.atEnd() && !conf.hasError())
	{
		if (AbstractInputHandler::parseMap.contains(conf.name().toString()))
		{
			qDebug() << "found <" << conf.name().toString(); 
			// all handler-specific configuration is read by the registered parsing functions
			handlers.append(AbstractInputHandler::parseMap[conf.name().toString()](conf));
			// FIXME: write a proper interface for doing this or something
			if (conf.name() == "xwatcher")
			{
				XWatcher *xwatcher = static_cast<XWatcher*>(handlers.last());
				connect(xwatcher, SIGNAL(windowChanged(QString,QString)),
						SLOT(activeWindowChanged(QString,QString)));
				connect(xwatcher, SIGNAL(windowTitleChanged(QString)),
						SLOT(activeWindowTitleChanged(QString)));
			}
		}
		conf.readNextStartElement();
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
	
	/// Now everything is prepared to read the Window-specific settings
	f.seek(0);
	conf.setDevice(&f);
	conf.readNextStartElement();
	while(!conf.atEnd() && !conf.hasError())
	{
		if (conf.name() == "window")
		{
			readWindowSettings(conf, ids);
		}
		conf.readNextStartElement();
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

bool InputMangler::readWindowSettings(QXmlStreamReader& conf, QStringList& ids)
{
	QString windowClass = conf.attributes().value("class").toString();
	QMap<QString, WindowSettings*> wsettings;
	QMap<QString, bool> used;
	// create WindowSettings
	foreach(QString id, ids)
	{
		wsettings.insert(id, new WindowSettings());
		used[id] = false;
		// set default outputs for that window class to ids default.
		wsettings[id]->def = wsets[id].def;
	}
	
	foreach (QXmlStreamAttribute attr, conf.attributes())
	{
		if (attr.name() == "class")
			 continue;
		else if (ids.contains(attr.name().toString()))
		{
			wsettings[attr.name().toString()]->def = parseOutputsShort(attr.value().toString());
			used[attr.name().toString()] = true;
		}
		else
			qDebug() << "Warning: unexpected attribute at line " << conf.lineNumber();
	}
	
	while(!conf.atEnd() && !conf.hasError())
	{
		if (conf.isEndElement())
		{
			if (conf.name() != "window")
				xmlError(conf);
			break;
		}
		if (!conf.isStartElement())
		{
			conf.readNext();
			continue;
		}
		if (conf.name() == "long")
		{
			WindowSettings *w = wsettings[conf.attributes().value("id").toString()];
			w->def = parseOutputsLong(conf, w->def);
		}
		if (conf.name() == "title")
		{
			QString regex = conf.attributes().value("regex").toString();
			foreach (QXmlStreamAttribute attr, conf.attributes())
			{
				if (attr.name().toString() == "regex")
					continue;
				else if (ids.contains(attr.name().toString()))
				{
					// attr.name == id
					wsettings[attr.name().toString()]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
					wsettings[attr.name().toString()]->events.append(parseOutputsShort(attr.value().toString()));
					used[attr.name().toString()] = true;
				}
				else
					qDebug() << "Reading <title> - Warning: unexpected attribute at line " << conf.lineNumber();
			}
			conf.readNext();
			while(!conf.atEnd() && !conf.hasError())
			{
				if (conf.isEndElement())
				{
					if (conf.name() == "title")
						break;
					if (conf.name() != "long")
						xmlError(conf);
					break;
				}
				if (!conf.isStartElement())
				{
					conf.readNext();
					continue;
				}
				
				if (conf.name() != "long")
				{
					qDebug() << "Reading <long> - Warning: unexpected element at line " << conf.lineNumber();
					conf.readNext();
					continue;
				}
				
				QString id = conf.attributes().value("id").toString();
				wsettings[id]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
				wsettings[id]->events.append(parseOutputsLong(conf, wsettings[id]->def));
				used[id] = true;
				conf.readNext();
			}
		}
		conf.readNext();
	}
	foreach(QString id, ids)
	{
		if (used[id] == true)
			wsets[id].addWindowSettings(windowClass, wsettings[id]);
		else
			delete wsettings[id];
	}
	return true;
}


void InputMangler::xmlError(QXmlStreamReader& conf)
{
	qDebug() << "Error in configuration at line " << conf.lineNumber();
}

QVector< OutEvent > InputMangler::parseOutputsShort(QString s)
{
	QVector<OutEvent> vec;
	QStringList l = s.split(",");
	foreach(QString s, l)
		vec.append(OutEvent(s));
	return vec;
}

/*!
 * @brief Parse output definition for TransformationStructure with id. 
 * Returns def when nothing is found.
 * @param id Id of a TransformationStructure
 * @param element XML element, typicaly <window> or <title>
 * @param def Default value. Returned when no output definition is found. Result of
 * <long> description is based on this.
 */
QVector< OutEvent > InputMangler::parseOutputsLong(QXmlStreamReader& conf, QVector< OutEvent > def)
{
	if (conf.readNext() != QXmlStreamReader::Characters)
	{
		xmlError(conf);
		return QVector<OutEvent>();
	}
	
	// valid seperators are '\n' and ','
	QStringList lines = conf.text().toString().split(QRegExp("(\n|,)"));
	foreach (QString s, lines)
	{
		// we want allow a dual use of '=', e.g. <long> ==c, r== </long>
		s = s.trimmed();
		int pos = s.indexOf('=', 1);
		QString left = s.left(pos);
		QString right = s.mid(pos + 1);
		if (left.isEmpty() && right.isEmpty())
			break;
	}
	if (conf.readNext() != QXmlStreamReader::EndElement)
		xmlError(conf);
	
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

