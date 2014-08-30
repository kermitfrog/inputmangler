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


#include "nethandler.h"
#include <QDebug>
#include <QTcpSocket>
#include <QString>
#include "keydefs.h"
#include <QTest>

/*!
 * @brief Constructs a NetHandler.
 * @param address IP or host address of the network device on which to listen on.
 * @param port The port.
 */
NetHandler::NetHandler(QString address, int port)
{
	addr = QHostAddress(address);
	this->port = port;
	this->sd = sd;
	_id = "___NET";
	_hasWindowSpecificSettings = false;
}

/*!
 * @brief
 * this thread will read text from the Network
 * and create events to write that text.
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
		if (sd.terminating)
		{
			s->close();
			delete s;
			return;
		}
		if (!state)
			continue;
		
		socket = s->nextPendingConnection();
		qDebug() << "Yay! Someone connected at " << QDateTime::currentDateTime();
		
		while(1)
		{
			state = socket->waitForReadyRead(1000);
			// check if we should stop the loop
			if (sd.terminating)
			{
				s->close();
				delete socket;
				delete s;
				return;
			}
			if (!state)
				if (socket->state() == QAbstractSocket::UnconnectedState)
				{
					qDebug() << "Nay! Someone disconnected at " << QDateTime::currentDateTime();
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

/*!
 * @brief Process the incoming data. Possibly unfinished input will be buffered.
 * @param b Input.
 * @param n Length of input.
 */
void NetHandler::actOnData(char* b, int n)
{
	// we need a buffer for split multi-char commands
	QString s = buffer + QString::fromLatin1(b, n);
	buffer = "";
//   	qDebug() << "Buffer: " << buffer << " S: " << s << "Hex:" << QTest::toHexRepresentation(b, n);
//   	qDebug() << "Hex:" << QTest::toHexRepresentation(b, n);
	for (int i = 0; i < s.length(); i++)
	{
		// clearly not a multi-char-sequence
		if (!sequence_starting_chars.contains(s[i]))
		{
			charmap[s[i].toLatin1()].send();
			continue;
		}
		// may be a multi-char-sequence, but possibly not complete -> put it into the buffer and end function
		if (s.length() - i < max_sequence_length)
		{
			buffer = s.mid(i);
			return;
		}
		// look if it is a multi-char-sequence
		int iMod = 0;
		for (int j = 2; j <= max_sequence_length; j++)
		{
			if (specialmap.contains(s.mid(i, j)))
			{
				specialmap[s.mid(i,j)].send();
				iMod = j - 1;
				break;
			}
		}
		// if it was a multi-char-sequence, iMod => 1. Increment i approparately and continue loop.
		if (iMod)
		{
			i += iMod;
			continue;
		}
		// nope, it's not a multi-char-sequence after all
		charmap[s[i].toLatin1()].send();
		//qDebug() << "Data: " << s[i];
	}
}

/*!
 * @brief Parses the corresponding xml parts and creates NetHandler objects.
 * @param nodes All the <net> nodes.
 * @return List containing all NetHandlers.
 */
QList< AbstractInputHandler* > NetHandler::parseXml(QDomNodeList nodes)
{
	QList<AbstractInputHandler*> handlers;
	/// nethandler
	NetHandler *n;
	for (int i = 0; i < nodes.length(); i++)
	{
	n = new NetHandler(	nodes.at(i).attributes().namedItem("addr").nodeValue(),
						nodes.at(i).attributes().namedItem("port").nodeValue().toInt() );
	handlers.append(n);
	}
	return handlers;
}


NetHandler::~NetHandler()
{
	

}

