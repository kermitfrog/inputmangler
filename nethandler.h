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


#ifndef NETHANDLER_H
#define NETHANDLER_H

#include "abstractinputhandler.h"
#include <QTcpServer>
#include <QHostAddress>

class NetHandler : public AbstractInputHandler
{

public:
    NetHandler(shared_data *sd, QString a, int port);
    virtual ~NetHandler();
	virtual void run();
	void actOnData(char *b, int n);
	
private:
	QTcpServer *s;
	QHostAddress addr;
	int port;
	QString buffer;
	TEvent l, r, dot;
};

#endif // NETHANDLER_H
