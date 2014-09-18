/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014 Arkadiusz Guzinski <kermit@ag.de1.cc>, [Name] <email>
   
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

#include "xwatcher.h"

/*!
 * @brief Constructor of XWatcher
 */
XWatcher::XWatcher()
{
	_id = "___XWATCHER";
	_hasWindowSpecificSettings = false;
	
// 	display = XOpenDisplay(NULL);
// 	if (!display)
// 		qFatal("connection to X Server failed");
}

/*!
 * @brief Parses a <xwatcher> part of the configuration and constructs a XWatcher object.
 * If XWatcher needs any Parameters, they should be read here.
 * See the parseXml functions of DevHandler, DebugHandler and NetHandler
 * for examples.
 * @param xml QXmlStreamReader object at current position of a <xwatcher> element.
 * @return List containing a XWatcher.
 */
QList< AbstractInputHandler* > XWatcher::parseXml(QXmlStreamReader &xml)
{
	QList<AbstractInputHandler*> handlers;
	/// xwatcher
	XWatcher *xw = new XWatcher();
	handlers.append(xw);
	return handlers;
}

XWatcher::~XWatcher()
{
	//XFree(display); // is that the right way to close the connection with X?
}

/*!
 * @brief Looks for changes of the current window & title.
 * This method should provide some way of getting changes of the current
 * window class (the 1st field shown with xprop | grep WM_CLASS)
 * and window title from X and emit a signal...
 * On class change:
 * emit windowChanged(QString wclass, QString title);
 * On title change:
 * emit windowTitleChanged(QString wtitle);
 * Also, it has to watch sd.terminating and end itself if that 
 * changes to true
 */
void XWatcher::run()
{
	qDebug() << "XWatcher not yet implemented";
	
}

#include "xwatcher.moc"
