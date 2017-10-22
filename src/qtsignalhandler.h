/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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
#include <QObject>
#include <QSocketNotifier>

/*!
 * @brief This class sends qt signals when the application receives a linux signal.
 */
class QtSignalHandler : public QObject
{
Q_OBJECT

	public:
	QtSignalHandler(QObject* parent = 0);
	~QtSignalHandler() {};

	static void signalHandler(int signum);
	
public slots:
	void socketReady(); 

signals:
	void hupReceived();
	void usr1Received();
	void usr2Received();

private:
	static int mySocketPairFd[2];
	QSocketNotifier *socNot;
	void socketEmitter(int signum);
	
};


