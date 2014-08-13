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


#include "qtsignalhandler.h"
#include <signal.h>
#include <QCoreApplication>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int QtSignalHandler::mySocketPairFd[2];

QtSignalHandler::QtSignalHandler(QObject* parent, const char* name): QObject(parent)
{
	socketpair(AF_UNIX, SOCK_STREAM, 0, mySocketPairFd);
	socNot = new QSocketNotifier(mySocketPairFd[1], QSocketNotifier::Read, this);
	connect(socNot, SIGNAL(activated(int)), SLOT(socketReady()));
	
	signal(SIGTERM, signalHandler);
	signal(SIGHUP, signalHandler);
	signal(SIGUSR1, signalHandler);
	signal(SIGUSR2, signalHandler);
	signal(SIGINT, signalHandler);
}

void QtSignalHandler::signalHandler(int signum)
{
	write(mySocketPairFd[0], &signum, sizeof(signum));
}

void QtSignalHandler::socketReady()
{
	int signum;
	read(mySocketPairFd[1], &signum, sizeof(signum));
	socketEmitter(signum);
}

void QtSignalHandler::socketEmitter(int signum)
{
	switch (signum)
	{
		case SIGTERM:
		case SIGINT:
			QCoreApplication::quit();
			break;
		case SIGHUP:
			emit hupReceived();
			break;
		case SIGUSR1:
			emit usr1Received();
			break;
		case SIGUSR2:
			emit usr2Received();
			break;
	}
}



#include "moc_qtsignalhandler.cpp"
