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


#include "devhandler.h"
#include "inputmangler.h"
#include <poll.h>       //poll
#include <QDebug>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>


void DevHandler::run()
{
	fd = open(filename.toLatin1(), O_RDONLY);
	if (fd == -1)
	{
		qDebug() << "?could (not?) open " << filename;
		return;
	}
	struct pollfd p;
	p.fd = fd;
	p.events = POLLIN;
	p.revents = POLLIN;
	
	int ret, n; int testval;

	input_event buf[4];
	VEvent e[NUM_MOD*2+2];
	while (1)
	{
		ret = poll (&p, 1, 1500);
		if(sd->terminating)
		{
			qDebug() << "terminating";
			break;
		}
		if(ret)
		{
			n = read(fd, buf, 4*sizeof(input_event));
			for (int i = 0; i < n/sizeof(input_event); i++)
			{
				for(int j = 0; j < outputs.size(); j++)
					if (buf[i].code == outputs.at(j).keycode)
					{
						// Combo Event
						if(outputs.at(j).modifiers.size())
						{
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
								sendKbdEvent(&e[m.size()+2], m.size());
							}
							else
								sendKbdEvent(e, m.size() * 2);
						}
						// Raw Event
						else
						{
							e[0].type = buf[i].value;
							e[0].code = outputs.at(j).code();
							e[0].value = buf[i].value;
							if (e[0].code >= BTN_MISC)
								sendMouseEvent(e);
							else
								sendKbdEvent(e);
						}
						//TODO: Autofire, Macros
					}
				//qDebug("type: %d, code: %d, value: %d", buf[i].type, buf[i].code, buf[i].value );
			}
			//qDebug() << id << ":" << n;
		}
	}
	
}

void DevHandler::createEvent(OutEvent* out, input_event* buf)
{

}



DevHandler::DevHandler(idevs i, shared_data *sd)
{
	this->sd = sd;
	_id = i.id;
	qDebug() << "id is " << i.id;
	filename = QString("/dev/input/") + i.event;

}

DevHandler::~DevHandler()
{

}

