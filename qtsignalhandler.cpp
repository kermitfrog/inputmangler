/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Arek <arek@ag.de1.cc>

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
#include <QCoreApplication>
#include <stdio.h>

int QtSignalHandler::sighupFd[2];
int QtSignalHandler::sigtermFd[2];

//Somewhere else in your startup code, you install your Unix signal handlers with sigaction(2).
static int setup_unix_signal_handlers()
{
	struct sigaction hup, term;

	hup.sa_handler = QtSignalHandler::hupSignalHandler;
	sigemptyset(&hup.sa_mask);
	hup.sa_flags = 0;
	hup.sa_flags |= SA_RESTART;

	if (sigaction(SIGHUP, &hup, 0) > 0)
		return 1;

	term.sa_handler = QtSignalHandler::termSignalHandler;
	sigemptyset(&term.sa_mask);
	term.sa_flags |= SA_RESTART;

	if (sigaction(SIGTERM, &term, 0) > 0)
		return 2;

	return 0;
}

QtSignalHandler::QtSignalHandler(QObject *parent, const char *name)
{
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
		qFatal("Couldn't create HUP socketpair");

	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
		qFatal("Couldn't create TERM socketpair");
	snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
	connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
	snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
	connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
	setup_unix_signal_handlers();
	
}

//In your Unix signal handlers, you write a byte to the write end of a socket pair and return. This will cause the corresponding QSocketNotifier to emit its activated() signal, which will in turn cause the appropriate Qt slot function to run.
void QtSignalHandler::hupSignalHandler(int)
{
	printf("--HUP--");
	char a = 1;
	::write(sighupFd[0], &a, sizeof(a));
}

void QtSignalHandler::termSignalHandler(int)
{
	printf("--TERM--");
	char a = 1;
	::write(sigtermFd[0], &a, sizeof(a));
}

//In the slot functions connected to the QSocketNotifier::activated() signals, you read the byte. Now you are safely back in Qt with your signal, and you can do all the Qt stuff you weren'tr allowed to do in the Unix signal handler.
void QtSignalHandler::handleSigTerm()
{
	snTerm->setEnabled(false);
	char tmp;
	::read(sigtermFd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	QCoreApplication::quit();

	snTerm->setEnabled(true);
}

void QtSignalHandler::handleSigHup()
{
	snHup->setEnabled(false);
	char tmp;
	::read(sighupFd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	emit hupReceived();

	snHup->setEnabled(true);
}

#include "moc_qtsignalhandler.cpp"
