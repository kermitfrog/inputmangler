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


#pragma once

#include "abstractinputhandler.h"
#include <QTcpServer>
#include <QHostAddress>

/*!
 * @brief NetHandler listens on a TCP Port and generates input events from the incoming data.
 */
class NetHandler : public AbstractInputHandler
{

public:
    NetHandler(QString address, int port);
	static QList<AbstractInputHandler*> parseXml(pugi::xml_node &xml);
    virtual ~NetHandler();
	virtual void run();
	virtual int getType() {return 100;};

protected:
	void actOnData(char *b, int n);
	QTcpServer *s;
	QHostAddress addr;
	int port;
	QString buffer; //!< Buffer for unfinished input sequences.
};

