/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Arkadiusz Guzinski <kermit@ag.de1.cc>

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


#include "devhandler.h"
#include "inputmangler.h"
#include <poll.h>       //poll
#include <QDebug>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
 * this thread will read from one input device and transform input
 * events according to previosly set rules
 */

void DevHandler::run()
{
	//TODO toLatin1 works... always? why not UTF-8?
	fd = open(filename.toLatin1(), O_RDONLY);
	if (fd == -1)
	{
		qDebug() << "?could (not?) open " << filename;
		return;
	}
	
	// grab the device, otherwise there will be double events
	ioctl(fd, EVIOCGRAB, 1);
	
	
	struct pollfd p;
	p.fd = fd;
	p.events = POLLIN;
	p.revents = POLLIN;
	
	int ret, n;
	bool matches;

	input_event buf[4];
	VEvent e[NUM_MOD*2+2];
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
				//if (id() == "M")// && buf[i].type == EV_REL && buf[i].code > 1 )
				//	qDebug()<< "yup " << buf[i].type << " " << buf[i].code << " " << buf[i].value;
				matches = false;
				// Key/Button events only
				// We do not yet handle mouse moves and
				// certainly not misc and sync events
				if(buf[i].type == EV_KEY) 
					for(int j = 0; j < outputs.size(); j++)
					{	
	// 					qDebug() << j << " of " << outputs.size() << " in " << id();
						if (buf[i].code == inputs[j])
						{
							matches = true;
#ifdef DEBUGME
							qDebug() << "Output : " << outputs.at(j).initString;
#endif
							// Combo Event ( see TEvent )
							if(outputs.at(j).modifiers.size())
							{
								if (buf[i].value == 0)
									break;
								QVector<__u16> m = outputs.at(j).modifiers;
								int offset = 2 + m.size(), k = 0;
								for (; k < m.size(); k++)
								{
									e[k].type = EV_KEY;
									e[k].code = m.at(k);
									e[k].value = 1;
									e[k+offset].type = EV_KEY;
									e[k+offset].code = m.at(k);
									e[k+offset].value = 0;
								}
								e[k].type = EV_KEY;
								e[k].code = outputs.at(j).keycode;
								e[k].value = 1;
								e[k+1].type = EV_KEY;
								e[k+1].code = outputs.at(j).keycode;
								e[k+1].value = 0;
								if (e[k].code >= BTN_MISC)
								{
									sendKbdEvent(e, m.size());
									sendMouseEvent(&e[m.size()], 2);
									sendKbdEvent(&e[offset], m.size());
								}
								else
									sendKbdEvent(e, m.size() * 2 + 2);
							}
							// Raw Event
							else
							{
								//qDebug() << "Raw " << outputs.at(j).code();
								e[0].type = buf[i].type;
								e[0].code = outputs.at(j).code();
								e[0].value = buf[i].value;
								if (e[0].code >= BTN_MISC)
									sendMouseEvent(e);
								else
									sendKbdEvent(e);
							}
							break;
							//TODO: Autofire, Macros
						}
					}
				// Pass through
				if (!matches)
				{
					e[0].type = buf[i].type;
					e[0].code = buf[i].code;
					e[0].value = buf[i].value;
					if (devtype == Mouse)
						sendMouseEvent(e);
					else
						sendKbdEvent(e);
				}
// 				qDebug("type: %d, code: %d, value: %d", buf[i].type, buf[i].code, buf[i].value );
			}
			//qDebug() << id << ":" << n;
		}
	}
	// unlock & close
	ioctl(fd, EVIOCGRAB, 0);
	close(fd);
	
}

DevHandler::DevHandler(idevs i)
{
	this->sd = sd;
	_id = i.id;
	hasWindowSpecificSettings = _id != "";
	filename = QString("/dev/input/") + i.event;
	if (i.mouse)
		devtype = Mouse;
	else
		devtype = Keyboard;
}

QList< AbstractInputHandler* > DevHandler::parseXml(QDomNodeList nodes)
{
	QList<AbstractInputHandler*> handlers;
	// get a list of available devices from /proc/bus/input/devices
	QList<idevs> availableDevices = parseInputDevices();
	/// Devices
	// for every configured <device...>
	for (int i = 0; i < nodes.length(); i++)
	{
		/*
		 * create an idevs structure for the configured device
		 * vendor and product are set to match the devices in /proc/bus/...
		 * id is the config-id 
		 */
		idevs d;
		d.vendor  = nodes.at(i).attributes().namedItem("vendor").nodeValue();
		d.product = nodes.at(i).attributes().namedItem("product").nodeValue();
		d.id      = nodes.at(i).attributes().namedItem("id").nodeValue();
		
		// create a devhandler for every device that matches vendor and product
		// of the configured device,
		while (availableDevices.count(d))
		{
			int idx = availableDevices.indexOf(d);
			// copy information obtained from /proc/bus/input/devices to complete
			// the data in the idevs object used to construct the DevHandler
			d.event = availableDevices.at(idx).event;
			d.mouse = availableDevices.at(idx).mouse;
			handlers.append(new DevHandler(d));
			availableDevices.removeAt(idx);
			
			/*
			 * read the <signal> entries.
			 * [key] will be the input event, that will be transformed
			 * [default] will be the current output device, this is 
			 * transformed to. If no [default] is set, the current output will
			 * be the same as the input.
			 * For DevHandlers with window specific settings, the current output
			 * becomes the default output when the TransformationStructure is
			 * constructed, otherwise it won't ever change anyway...
			 */
			QDomNodeList codes = nodes.at(i).toElement().elementsByTagName("signal");
			for (int j = 0; j < codes.length(); j++)
			{
				QString key = codes.at(j).attributes().namedItem("key").nodeValue();
				QString def = codes.at(j).attributes().namedItem("default").nodeValue();
				if (def == "")
					handlers.last()->addInputCode(keymap[key]);
				else
					handlers.last()->addInputCode(keymap[key], OutEvent(def));
			}
		}
	}
	return handlers;
}

DevHandler::~DevHandler()
{

}

