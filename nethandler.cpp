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
}

void NetHandler::run()
{
	s = new QTcpServer();
	if (!s->listen(QHostAddress(addr), port))
		qDebug() << "listening on " << addr.toString() << ":" << port << " failed";
	char b[1024];
	int n;
	bool state;
	QTcpSocket *t = NULL;
	while (1)
	{
		state = s->waitForNewConnection(1500);
		if (sd->terminating)
		{
			if (t)
			{
				t->disconnectFromHost();
				t->close();
			}
			s->close();
			return;
		}
		if (!state)
			continue;
		
		t = s->nextPendingConnection();
		qDebug() << "Yay! Someone connected";
		
		while(1)
		{
			state = t->waitForReadyRead(1500);
			if (sd->terminating)
			{
				t->disconnectFromHost();
				t->close();
				s->close();
				delete t;
				delete s;
				return;
			}
			if (!state)
				if (t->state() == QAbstractSocket::UnconnectedState)
				{
					qDebug() << "Nay! Someone disconnected";
					break;
				}
				else
					continue;
			
			while (n = t->bytesAvailable())
			{
				t->read(b, 1016);
				actOnData(b, n);
			}	
		}
		delete t;
	}
}

void NetHandler::actOnData(char* b, int n)
{
	QString s = buffer + QString::fromLatin1(b, n);
	buffer = "";
	qDebug() << "Buffer: " << buffer << " S: " << s << "Hex:" << QTest::toHexRepresentation(b, n);
// 	bool shift = false, altGr = false;
	for (int i = 0; i < n; i++)
	{
		if (s[i] == '\x085') //'...'
		{
			int dc = 3;
			while(dc--)
				sendTextEvent(&dot);
			continue;
		}	
		if (s[i] == '\x01b')
		{
			if (s.length() - i < 3)
			{
				buffer = s;
				return;
			}
			i+=2;
			if(s[i] == '\x044')
				sendTextEvent(&l);
			else
				sendTextEvent(&r);
			continue;
		}
 		if (s[i] == '^')
		{
			if (s.length() - i < 2) 
			{
				buffer = s;
				return;
			}
#ifdef DEBUGME
			if (s[i+1] == 'c')
				qDebug () << "Copy";
#endif
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

