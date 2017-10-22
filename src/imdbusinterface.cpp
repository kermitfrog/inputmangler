/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2017  Arkadiusz Guzinski <kermit@ag.de1.cc>

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


#include "imdbusinterface.h"

/*!
 * @brief Register on d-bus.
 */
imDbusInterface::imDbusInterface(InputMangler* im)
{
	this->im = im;
 	QDBusConnection::sessionBus().registerService("org.inputManglerInterface");
 	QDBusConnection::sessionBus().registerObject("/org/inputMangler/Interface", this, QDBusConnection::ExportAllSlots);
}

imDbusInterface::~imDbusInterface()
{
	QDBusConnection::sessionBus().unregisterObject("/org/inputMangler/Interface", QDBusConnection::UnregisterTree);
	QDBusConnection::sessionBus().unregisterService("org.inputManglerInterface");
}



// #include "moc_imdbusinterface.cpp"
