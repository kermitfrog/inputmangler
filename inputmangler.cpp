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
			connect(static_cast<XWatcher*>(handlers.last()), SIGNAL(windowChanged(QString,QString)),
					SLOT(activeWindowChanged(QString,QString)));
			connect(static_cast<XWatcher*>(handlers.last()), SIGNAL(windowTitleChanged(QString)),
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
	qDebug() << "hasSettingsForId(" << id;
	if (element.hasAttribute(id))
		return true;
	qDebug() << "1";
	QDomNodeList nodes = element.elementsByTagName("long");
	for (int i = 0; i < nodes.length(); i++)
		if (nodes.at(i).attributes().namedItem("id").nodeValue() == id)
			return true;
	qDebug() << "2";
	return false;
}

QVector< OutEvent > InputMangler::parseOutputs(QString id, QDomElement element, QVector< OutEvent > def)
{
	if (element.hasAttribute(id))
		return parseOutputsShort(element.attributes().namedItem(id).nodeValue());
	qDebug() << "long";
	QDomNodeList nodes = element.elementsByTagName("long");
	for(int i =  0; i < nodes.length(); i++)
	{
		if (nodes.at(i).attributes().namedItem("id").nodeValue() != id)
			continue;
		qDebug() << "i = " << i;
		QStringList lines = nodes.at(i).toElement().text().split("\n");
		qDebug() << lines;
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
	foreach (AbstractInputHandler *h, handlers)
	{
		if (h->id() == "" && h->getNumInputs() == h->getNumOutputs() && h->getNumInputs())
			continue;
		if(!h->hasWindowSpecificSettings )
			continue;
		wsets[h->id()].sanityCheck(h->getNumInputs(), h->id(), true);
	}
}

InputMangler::~InputMangler()
{
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
bool TransformationStructure::sanityCheck(int s, QString id, bool debug)
{
	bool result = true;
	qDebug() << "\nchecking " << id << " with size " << s;
	if (this->def.size() != s)
	{
		qDebug() << "TransformationStructure.def is " << this->def.size();
		result = false;
	}
	QList<WindowSettings*> wlist = classes.values();
	foreach (WindowSettings *w, wlist)
	{
		if (debug)
		{
			qDebug() << "Settings for Window = " << classes.key(w);
			QString s = "  ";
			for (int j = 0; j < w->def.size(); j++)
			{
				s += w->def.at(j).print();
				if (j < w->def.size() - 1)
					s += ", ";
			}
			qDebug() << s;
		}
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
			if (debug)
			{
				qDebug() << "  with Pattern: \"" << w->titles[i]->pattern() << "\"";
				QString s = "    ";
				for (int j = 0; j < w->events.at(i).size(); j++)
				{
					s += w->events.at(i).at(j).print();
					if (j < w->events.at(i).size() - 1)
						s += ", ";
				}
				qDebug() << s;
			}
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

QString OutEvent::print() const
{
	QString s = keymap_reverse[keycode] + "(" + QString::number(keycode) + ")[";
		for(int i = 0; i < modifiers.count(); i++)
		{
			s += keymap_reverse[modifiers[i]] + " (" + QString::number(modifiers[i]) + ")";
			if (i < modifiers.count() - 1)
				s += ", ";
		}
		return s + "]";
}
