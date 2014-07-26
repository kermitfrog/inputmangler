/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013 
    TODO: this is 95% from the internet... where?

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
int QtSignalHandler::sigusr1Fd[2];
int QtSignalHandler::sigusr2Fd[2];

//Somewhere else in your startup code, you install your Unix signal handlers with sigaction(2).
static int setup_unix_signal_handlers()
{
	struct sigaction hup, term, usr1, usr2;

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

	usr1.sa_handler = QtSignalHandler::usr1SignalHandler;
	sigemptyset(&usr1.sa_mask);
	usr1.sa_flags |= SA_RESTART;

	if (sigaction(SIGUSR1, &usr1, 0) > 0)
		return 2;

	usr2.sa_handler = QtSignalHandler::usr2SignalHandler;
	sigemptyset(&usr2.sa_mask);
	usr2.sa_flags |= SA_RESTART;

	if (sigaction(SIGUSR2, &usr2, 0) > 0)
		return 2;

	return 0;
}

QtSignalHandler::QtSignalHandler(QObject *parent, const char *name)
{
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
		qFatal("Couldn't create HUP socketpair");

	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
		qFatal("Couldn't create TERM socketpair");
	
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr1Fd))
		qFatal("Couldn't create USR1 socketpair");
	
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr2Fd))
		qFatal("Couldn't create USR2 socketpair");
	
	snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
	connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
	snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
	connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
	snUsr1 = new QSocketNotifier(sigusr1Fd[1], QSocketNotifier::Read, this);
	connect(snUsr1, SIGNAL(activated(int)), this, SLOT(handleSigUsr1()));
	snUsr2 = new QSocketNotifier(sigusr2Fd[1], QSocketNotifier::Read, this);
	connect(snUsr2, SIGNAL(activated(int)), this, SLOT(handleSigUsr2()));
	setup_unix_signal_handlers();
	
}

//In your Unix signal handlers, you write a byte to the write end of a socket pair and return. This will cause the corresponding QSocketNotifier to emit its activated() signal, which will in turn cause the appropriate Qt slot function to run.
void QtSignalHandler::hupSignalHandler(int)
{
	char a = 1;
	::write(sighupFd[0], &a, sizeof(a));
}

void QtSignalHandler::termSignalHandler(int)
{
	char a = 1;
	::write(sigtermFd[0], &a, sizeof(a));
}

void QtSignalHandler::usr1SignalHandler(int)
{
	char a = 1;
	::write(sigusr1Fd[0], &a, sizeof(a));
}

void QtSignalHandler::usr2SignalHandler(int)
{
	char a = 1;
	::write(sigusr2Fd[0], &a, sizeof(a));
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

void QtSignalHandler::handleSigUsr1()
{
	snUsr1->setEnabled(false);
	char tmp;
	::read(sigusr1Fd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	emit usr1Received();

	snUsr1->setEnabled(true);
}

void QtSignalHandler::handleSigUsr2()
{
	snUsr2->setEnabled(false);
	char tmp;
	::read(sigusr2Fd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	emit usr2Received();

	snUsr2->setEnabled(true);
}


#include "moc_qtsignalhandler.cpp"
