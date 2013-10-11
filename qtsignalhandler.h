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


#ifndef QTSIGNALHANDLER_H
#define QTSIGNALHANDLER_H

#include <QSocketNotifier>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

class QtSignalHandler : public QObject
{
Q_OBJECT

   public:
     QtSignalHandler(QObject *parent = 0, const char *name = 0);
     ~QtSignalHandler() {};

     // Unix signal handlers.
     static void hupSignalHandler(int unused);
     static void termSignalHandler(int unused);

   public slots:
     // Qt signal handlers.
     void handleSigHup();
     void handleSigTerm();

   private:
     static int sighupFd[2];
     static int sigtermFd[2];

     QSocketNotifier *snHup;
     QSocketNotifier *snTerm;
};


#endif // QTSIGNALHANDLER_H
