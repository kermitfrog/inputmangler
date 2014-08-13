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

#include <QCoreApplication>
#include "inputmangler.h"
#include "qtsignalhandler.h"
#include "imdbusinterface.h"


int main(int argc, char **argv) {
	QCoreApplication a(argc,argv);
	imDbusInterface dbus;	//for some unknown reason it has to be constructed in main...
	InputMangler im;
	QtSignalHandler s;		//handles TERM, HUP, USR1 and USR2
	QObject::connect(&dbus, SIGNAL(windowChanged(QString, QString)),
					 &im, SLOT(activeWindowChanged(QString, QString)));
	QObject::connect(&dbus, SIGNAL(windowTitleChanged(QString)), 
					 &im, SLOT(activeWindowTitleChanged(QString)));
	QObject::connect(&s, SIGNAL(hupReceived()), 
					 &im, SLOT(reReadConfig()));
	QObject::connect(&a, SIGNAL(aboutToQuit()), 
					 &im, SLOT(cleanUp()));
	QObject::connect(&s, SIGNAL(usr1Received()),
					 &im, SLOT(printWinInfo()));
	QObject::connect(&s, SIGNAL(usr2Received()),
					 &im, SLOT(printConfig()));
	a.exec();
    return 0;
}
