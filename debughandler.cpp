/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "debughandler.h"
#include "inputmangler.h"
#include <poll.h>       //poll
#include <QDebug>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cerrno>

DebugHandler::DebugHandler(idevs i, QString out, bool grab)
{
	_id = i.id;
	_grab = grab;
	hasWindowSpecificSettings = _id != "";
	filename = QString("/dev/input/") + i.event;
	qDebug() << filename;
	outfile.setFileName(out);
}

DebugHandler::~DebugHandler()
{
}

void DebugHandler::run()
{
	//TODO toLatin1 works... always? why not UTF-8?
	fd = open(filename.toLatin1(), O_RDONLY);
	if (fd == -1)
	{
		qDebug() << "?could (not?) open " << filename;
		return;
	}
	
	// grab the device, otherwise there will be double events
	if (_grab)
		ioctl(fd, EVIOCGRAB, 1);
	
	
	struct pollfd p;
	p.fd = fd;
	p.events = POLLIN;
	p.revents = POLLIN;
	outfile.open(QIODevice::Append);
	QTextStream out(&outfile);
	
	int ret, n;

	input_event buf[4]; // 14 | 7 | 7| 12 | 
	out << "\n\nOn " << QDate::currentDate().toString() << " " << _id << ":\nTime          |  Type |  Code |      Value | KeyName";
	
	while (1)
	{
		//wait until there is data
		ret = poll (&p, 1, 1500);
		if (p.revents & ( POLLERR | POLLHUP | POLLNVAL ))
		{
			qDebug() << "Device " << _id << "was removed or other error occured!"
					 << "\n shutting down " << _id;
			break;
		}
		//break the loop if we want to stop
		if(sd.terminating)
		{
			qDebug() << "terminating";
			break;
		}
		//if we did not wake up due to timeout...
		if(ret)
		{
			n = read(fd, buf, 4*sizeof(input_event));
			for (int i = 0; i < n/sizeof(input_event); i++)
			{
				QString s = "\n";
				s += QTime::currentTime().toString("HH:mm:ss.zzz  |")
				  +  QString::number(buf[i].type).rightJustified(6)
				  +  " |"
				  +  QString::number(buf[i].code).rightJustified(6)
				  +  " |"
				  +  QString::number(buf[i].value).rightJustified(11)
				  +  " |";
				if (buf[i].type == 1)
				  s += " " + keymap_reverse[buf[i].code];
				out << s;
			}
			out.flush();
		}
	}
	// unlock & close
	if(_grab)
		ioctl(fd, EVIOCGRAB, 0);
	close(fd);
	outfile.close();
}

QList< AbstractInputHandler* > DebugHandler::parseXml(QDomNodeList nodes)
{
	QList<AbstractInputHandler*> handlers;
	QList<idevs> availableDevices = parseInputDevices();
	/// debug dump, aka keylogger
	DebugHandler *dh;
	for (int i = 0; i < nodes.length(); i++)
	{
		idevs d;
		d.vendor  = nodes.at(i).attributes().namedItem("vendor").nodeValue();
		d.product = nodes.at(i).attributes().namedItem("product").nodeValue();
		d.id      = nodes.at(i).attributes().namedItem("id").nodeValue();
		while (availableDevices.count(d))
		{
			int idx = availableDevices.indexOf(d);
			// copy information obtained from /proc/bus/input/devices to complete
			// the data in the idevs object used to construct the DevHandler
			d.event = availableDevices.at(idx).event;
			d.mouse = availableDevices.at(idx).mouse;
			handlers.append(new DebugHandler ( d,
							nodes.at(i).attributes().namedItem("log").nodeValue(), 
						    nodes.at(i).attributes().namedItem("grab").nodeValue().toInt()
											 ));
			availableDevices.removeAt(idx);
		}
	}
	return handlers;
}


#include "debughandler.moc"
