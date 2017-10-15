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


#pragma once

#include <QtDBus>
#include <QCoreApplication>
#include "inputmangler.h"
#include "output/outevent.h"

/*!
 * @brief Inputmanglers d-bus interface.
 */
class imDbusInterface : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.inputMangler.API.Interface")

	
public:
    explicit imDbusInterface(InputMangler *im);
    virtual ~imDbusInterface();

public slots:
	Q_NOREPLY void activeWindowChanged(QByteArray wclass, QString title) {
		emit windowChanged(QString(wclass), title);
	};
	Q_NOREPLY void activeWindowTitleChanged(QString wclass) {
		emit windowTitleChanged(wclass);
	};
	Q_NOREPLY void sendRaw(int type, int code, int value, int dtype)
	{
		OutEvent::sendRawSafe(type, code, value, static_cast<DType>(dtype));
	};
	Q_NOREPLY void sendRawString(QString type, QString code, QString value, QString dtype)
	{
		OutEvent::sendRawSafe(type.toUShort(), code.toUShort(), value.toUInt(), static_cast<DType>(dtype.toInt()));
		// TODO: extend, so that names can be used
	};
	QString printWinInfo()
	{
		return im->winInfoToString();
	};

signals:
	void windowChanged(QString wclass, QString title);
	void windowTitleChanged(QString wclass);

private:
	InputMangler * im;
};



