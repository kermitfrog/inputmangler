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

#include "imdbusinterface.h"
#include "handlers/handlers.h"
#include "ConfParser.h"

/*!
 * @brief Constructor. Calls registerHandlers and readConf.
 */
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
    // open output devices and set up some global variables.
    AbstractInputHandler::generalSetup();
	ConfParser confParser(&handlers, &wsets);
	OutEvent::generalSetup(confParser.getInputBits());

    foreach (AbstractInputHandler* handler, handlers)
    {
        if (handler->id() != "")
            handlersById.insert(handler->id(), handler);
        // FIXME: write a proper interface for doing this or something
        if (handler->getType() == 1000)// TODO clean code here...
        {
            XWatcher *xwatcher = static_cast<XWatcher*>(handlers.last());
            connect(xwatcher, SIGNAL(windowChanged(QString,QString)),
                    SLOT(activeWindowChanged(QString,QString)));
            connect(xwatcher, SIGNAL(windowTitleChanged(QString)),
                    SLOT(activeWindowTitleChanged(QString)));
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

	return true;
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
	OutEvent::closeVirtualDevices();
	foreach (AbstractInputHandler *h, handlers)
		delete h;
	handlers.clear();
	wsets.clear();
}

/*!
 * @brief Print the current window class and title to console.
 * Called by sending a USR1 signal.
 */
void InputMangler::printWinInfo()
{
	qDebug() << winInfoToString();
}

/*!
 * @brief get a string describing the current window
 */
QString InputMangler::winInfoToString()
{
	return "class = \"" + wm_class + "\", title = \"" + wm_title + "\"";
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

