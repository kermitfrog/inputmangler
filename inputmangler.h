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


#ifndef INPUTMANGLER_H
#define INPUTMANGLER_H

class AbstractInputHandler;

#include <qobject.h>
#include "abstractinputhandler.h"

struct shared_data
{
	int fd_kbd;
	int fd_mouse;
	bool terminating;
};

class idevs 
{
public:
	QString vendor;
	QString product;
	QString event;
	QString id;
	bool operator==(idevs o) const{
		return (vendor == o.vendor && product == o.product);
	};
};


class InputMangler : public QObject
{
	Q_OBJECT
public:
	InputMangler();
	virtual ~InputMangler();

public slots:
	void cleanUp();
	
private:
	shared_data * sd;
	QList<AbstractInputHandler*> handlers;
	QList<idevs> parseInputDevices();
};

#endif // INPUTMANGLER_H
