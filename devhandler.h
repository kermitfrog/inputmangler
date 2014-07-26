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


#pragma once

#include "abstractinputhandler.h"
#include <QFile>

class DevHandler : public AbstractInputHandler
{
	// does this handle a mouse or a keyboard?
	// for devices that do both, 2 instances are created
	enum DType{Keyboard, Mouse};

public:
    DevHandler(idevs i);
    virtual ~DevHandler();
	//setTranslations()
	static QList<AbstractInputHandler*> parseXml(QDomNodeList nodes);

private:
	QString filename;
	int fd;
	DType devtype;
	void run();
    void createEvent(OutEvent *out, input_event *buf);
};

