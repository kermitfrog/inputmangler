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

#pragma once

#include "abstractinputhandler.h"
// #include <X11/Xlib.h>
// #include <X11/Xutil.h>

/**
 * skeleton class, in case someone wants to write the code to get the current window and title from X.
 * I, most certainly, will not.
 */
class XWatcher : public AbstractInputHandler {
Q_OBJECT

public:
    XWatcher();

    ~XWatcher();

    static QList<AbstractInputHandler *> parseXml(pugi::xml_node &xml);

    virtual int getType() { return 1000; };

signals:

    void windowChanged(QString wclass, QString title);

    void windowTitleChanged(QString wtitle);

protected:
    virtual void run();

private:
// 	Display *display;

};

