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


#include "devhandler.h"
#include "inputmangler.h"
#include <QDebug>
#include <poll.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*!
 * @brief This thread will read from one input device and transform input
 * events according to previosly set rules.
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
	
	int ret; // return value of poll
	int n;   // number of events to read / act upon
	bool matches; // does it match an input code that we want to act upon?

	input_event buf[4];
	while (1)
	{
		//wait until there is data or 1.5 seconds have passed to look at sd.terminating 
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
				// Key/Button and movements(relative and absolute) only
				// We do not handle misc and sync events
				if(buf[i].type >= EV_KEY && buf[i].type <= EV_ABS) 
					for(int j = 0; j < outputs.size(); j++)
					{	
	 					//qDebug() << j << " of " << outputs.size() << " in " << id();
						if (inputs[j] == buf[i])
						{
							matches = true;
#ifdef DEBUGME
							qDebug() << "Output : " << outputs.at(j).initString;
#endif
							outputs[j].send(buf[i].value, buf[i].type);
							break;
						}
					}
				// Pass through
				if (!matches)
					OutEvent::sendRaw(buf[i].type, buf[i].code, buf[i].value, devtype);
// 				qDebug("type: %d, code: %d, value: %d", buf[i].type, buf[i].code, buf[i].value );
			}
			//qDebug() << id << ":" << n;
		}
	}
	// unlock & close
	ioctl(fd, EVIOCGRAB, 0);
	close(fd);
	
}

/*!
 * @brief Construct a DevHandler thread for the device described in device.
 * @param device Description of a device.
 */
DevHandler::DevHandler(idevs device)
{
	this->sd = sd;
	_id = device.id;
	_hasWindowSpecificSettings = _id != "";
	filename = QString("/dev/input/") + device.event;
	if (device.mouse)
		devtype = Mouse;
	else
		devtype = Keyboard;
}

/*!
 * @brief Parses all the <device> parts of the configuration and constructs DevHandler objects.
 * @param nodes All the <device> nodes.
 * @return List containing all DevHandlers.
 */
QList< AbstractInputHandler* > DevHandler::parseXml(QXmlStreamReader &xml)
{
	QList<AbstractInputHandler*> handlers;
	// get a list of available devices from /proc/bus/input/devices
	QList<idevs> availableDevices = parseInputDevices();
	/// Devices
	/*
	 * create an idevs structure for the configured device
	 * vendor and product are set to match the devices in /proc/bus/...
	 * id is the config-id 
	 */
	idevs d;
	d.vendor  = xml.attributes().value("vendor").toString();
	d.product = xml.attributes().value("product").toString();
	d.id      = xml.attributes().value("id").toString();
	
	// create a devhandler for every device that matches vendor and product
	// of the configured device,
	while (availableDevices.count(d))
	{
		int idx = availableDevices.indexOf(d);
		// copy information obtained from /proc/bus/input/devices to complete
		// the data in the idevs object used to construct the DevHandler
		d.event = availableDevices.at(idx).event;
		d.mouse = availableDevices.at(idx).mouse;
		DevHandler *devhandler = new DevHandler(d);
		availableDevices.removeAt(idx);
		handlers.append(devhandler);
	}
		
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
	xml.readNextStartElement();
	while(!xml.atEnd() && !xml.hasError())
	{
		if (xml.isStartElement())
		{
			if (xml.name() == "signal")
			{
				QString key = xml.attributes().value("key").toString();
				QString def = xml.attributes().value("default").toString();
				foreach(AbstractInputHandler *a, handlers)
				{
					DevHandler *devhandler = static_cast<DevHandler*>(a);
					if (def == "")
						devhandler->addInput(keymap[key]);
					else
						devhandler->addInput(keymap[key], OutEvent(def));
				}
				while(!xml.atEnd() && !xml.hasError())
				{
					if (xml.isEndElement())
						if (xml.name() == "signal")
							break;
						else
							qDebug() << "Reading a <signal> - Warning: unexpected end of element at line " << xml.lineNumber();
					xml.readNext();
				}
			}
			else
				qDebug() << "Reading a <device> - Warning: unexpected element at line " << xml.lineNumber();
		}
		else if (xml.isEndElement())
			if (xml.name() == "device")
				break;
			else
				qDebug() << "Reading a <device> - Warning: unexpected end of element at line " << xml.lineNumber();
		xml.readNext();
	}
	return handlers;
}

DevHandler::~DevHandler()
{

}

