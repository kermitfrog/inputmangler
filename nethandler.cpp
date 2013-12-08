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


#include "nethandler.h"
#include <QDebug>
#include <QTcpSocket>
#include <QString>
#include "keydefs.h"
#include <QTest>

NetHandler::NetHandler(shared_data *sd, QString a, int port)
{
	addr = QHostAddress(a);
	this->port = port;
	this->sd = sd;
	l = TEvent(KEY_LEFT);
	r = TEvent(KEY_RIGHT);
	dot = TEvent(KEY_E);
	_id = "___NET";
	hasWindowSpecificSettings = false;
}

/*
 * this thread will read text (and a few editing/movement commands) from the Network
 * and create events to write that text
 * it is intended to be used with speech recognition running in a virtual machine,
 * piping its output through hyperterminal with a few macros set..
 */

void NetHandler::run()
{
	s = new QTcpServer();
	if (!s->listen(QHostAddress(addr), port))
	{
		qDebug() << "listening on " << addr.toString() << ":" << port << " failed";
		delete s;
		return;
	}
	char b[1024]; // buffer
	int n;        // bytes read
	bool state;
	QTcpSocket *socket = NULL;
	while (1)
	{
		state = s->waitForNewConnection(1000);
		// check if we should stop the loop
		if (sd->terminating)
		{
			if (socket)
			{
				socket->disconnectFromHost();
				socket->close();
			}
			s->close();
			return;
		}
		if (!state)
			continue;
		
		socket = s->nextPendingConnection();
		qDebug() << "Yay! Someone connected";
		
		while(1)
		{
			state = socket->waitForReadyRead(1000);
			// check if we should stop the loop
			if (sd->terminating)
			{
				s->close();
				socket->disconnectFromHost();
				socket->close();
				delete socket;
				delete s;
				return;
			}
			if (!state)
				if (socket->state() == QAbstractSocket::UnconnectedState)
				{
					qDebug() << "Nay! Someone disconnected";
					break;
				}
				else
					continue;
			
			while (socket->bytesAvailable())
			{
				n = socket->read(b, 1016);
				actOnData(b, n);
			}	
		}
		delete socket;
	}
}

void NetHandler::actOnData(char* b, int n)
{
	// we need a buffer for split multi-char commands
	QString s = buffer + QString::fromLatin1(b, n);
	buffer = "";
// 	qDebug() << "Buffer: " << buffer << " S: " << s << "Hex:" << QTest::toHexRepresentation(b, n);
	for (int i = 0; i < s.length(); i++)
	{
		if (s[i] == '\x085') //'...'
		{
			int dc = 3;
			while(dc--)
				sendTextEvent(&dot);
			continue;
		}	
		if (s[i] == '\x01b') // Escaped ^[C (Cursor Right), or ^[D (Cursor Left)
		{
			if (s.length() - i < 3)
			{
				buffer = s.mid(i);
				return;
			}
			i+=2;
			if(s[i] == '\x044')
				sendTextEvent(&l); // left
			else
				sendTextEvent(&r); // right
			continue;
		}
 		if (s[i] == '^') // custom commands
		{
			if (s.length() - i < 2) 
			{
				buffer = s.mid(i);
				return;
			}
			sendTextEvent(&specialmap[s.mid(i,2)]);
			i++;
			continue;
		}
		//qDebug() << "Data: " << s[i];
		sendTextEvent(&(charmap[s[i].toLatin1()]));
		
	}
}



NetHandler::~NetHandler()
{

}

