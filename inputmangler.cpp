/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Arkadiusz Guzinski <kermit@ag.de1.cc>

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
#include "handlers.h"
// #include <fcntl.h>
#include <unistd.h>
#include <QtXml>
#include <signal.h>
#include "keydefs.h"


InputMangler::InputMangler()
{
	// set up key definitions
	setUpKeymaps(); // keys as well as char mappings and multi-char commands (nethandler)
	registerHandlers();
	AbstractInputHandler::generalSetup();
	readConf();
}

// Reads the Config and actually starts the handler threads, too
bool InputMangler::readConf()
{
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
			if (!hasSettingsForId(id, e))
			{
				for (int k = 0; k < titles.length(); k++)
				{
					QDomElement t = titles.at(k).toElement();
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
			WindowSettings *windowSetting = wsets[id].window(e.attribute("class"), true);
			/*
			 * set default outputs for that window class.
			 * in the special case mentioned above, the window default output
			 * events are same as the id default.
			 */
			if (hasNoIdInWindowButTitlesWithId)
				windowSetting->def = wsets[id].def;
			else
				windowSetting->def = parseOutputs(id, e, wsets[id].def);

			// for each <title> inside the current <window>
			for (int k = 0; k < titles.length(); k++)
			{
				QDomElement t = titles.at(k).toElement();
				if (!t.hasAttribute(id))
					continue;
				windowSetting->titles.append(new QRegularExpression(QString("^") + t.attribute("regex") + "$"));
				windowSetting->events.append(parseOutputs(id, t, windowSetting->def));
			}
		}	
	}
	
	// prepare and actualy start threads. also roll a sanity check
	AbstractInputHandler::sd.terminating = false;
	
	activeWindowTitleChanged("");
	bool sane;
	foreach (AbstractInputHandler *h, handlers)
	{
		sane = false;
		if (h->id() == "" && h->getNumInputs() == h->getNumOutputs() && h->getNumInputs())
			sane = true;
		else if(!h->hasWindowSpecificSettings )
			sane = true;
		else 
			sane = wsets[h->id()].sanityCheck(h->getNumInputs(), h->id());
		
		if (sane)
			h->start();
	}
	
	f.close();
}

bool InputMangler::hasSettingsForId(QString id, QDomElement element)
{
	if (element.hasAttribute(id))
		return true;
	QDomNodeList nodes = element.elementsByTagName("long");
	for (int i = 0; i < nodes.length(); i++)
		if (nodes.at(i).attributes().namedItem("id").nodeValue() == id)
			return true;
	return false;
}

QVector< OutEvent > InputMangler::parseOutputs(QString id, QDomElement element, QVector< OutEvent > def)
{
	if (element.hasAttribute(id))
		return parseOutputsShort(element.attributes().namedItem(id).nodeValue());
	QDomNodeList nodes = element.elementsByTagName("long");
	for(int i =  0; i < nodes.length(); i++)
	{
		if (nodes.at(i).attributes().namedItem("id").nodeValue() != id)
			continue;
		QStringList lines = nodes.at(i).toElement().text().split(QRegExp("(\n|,)"));
		foreach (QString s, lines)
		{
			QStringList args = s.trimmed().split(" ");
			if (args.count() != 2)
				return def;
			
			QMap<QString,AbstractInputHandler*>::iterator handler = handlersById.find(id);
			while(handler != handlersById.end() && handler.key() == id)
			{
				int idx = handler.value()->inputIndex(args[0]);
				if (idx >= 0)
				{
					def[idx] = OutEvent(args[1]);
					break;
				}
				++handler;
			}
		}
	}
	return def;
}

QVector< OutEvent > InputMangler::parseOutputsShort(QString confString)
{
	QVector<OutEvent> out;
	QStringList l = confString.split(",");
	foreach(QString s, l)
		out.append(OutEvent(s));
	return out;
}



/*
 * this is called when the active window changes
 * it updates the Outputs of all handlers
 */
void InputMangler::activeWindowChanged(QString wclass, QString title)
{
//	qDebug() << "InputMangler::activeWindowChanged(" << wclass << ", " << title << ")";
	if (AbstractInputHandler::sd.terminating)
		return;
		
	wm_class = wclass;
	wm_title = title;
	
	foreach (AbstractInputHandler *a, handlers)
		if (a->hasWindowSpecificSettings)
			a->setOutputs(wsets[a->id()].getOutputs(wm_class, wm_title));
	
}

/*
 * this is called when the title of the active window changes
 * it just calls activeWindowChanged with the current class
 * and the new title
 */
void InputMangler::activeWindowTitleChanged(QString title)
{
//	qDebug() << "InputMangler::activeWindowTitleChanged(" << title << ")";
	activeWindowChanged(wm_class, title);
}

/*
 * end threads and close all devices
 */
void InputMangler::cleanUp()
{
	AbstractInputHandler::sd.terminating = true;
	qDebug() << "waiting for Threads to finish";
	foreach (AbstractInputHandler *h, handlers)
		h->wait(4000);
	close(AbstractInputHandler::sd.fd_kbd);
	close(AbstractInputHandler::sd.fd_mouse);
	foreach (AbstractInputHandler *h, handlers)
		delete h;
	handlers.clear();
	wsets.clear();
}

void InputMangler::printWinInfo()
{
	qDebug() << "Class = " << wm_class << ", Title = " << wm_title;
}

void InputMangler::printConfig()
{
	QStringList checkedIds;
	foreach (AbstractInputHandler *h, handlers)
	{
		if(!h->hasWindowSpecificSettings || h->id() == "")
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

void InputMangler::reReadConfig()
{
	cleanUp();
	readConf();
}

