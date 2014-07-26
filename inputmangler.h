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
#include "keydefs.h"

class AbstractInputHandler;

#include <qobject.h>
#include <QtDBus/QtDBus>
#include <QtXml>
#include "abstractinputhandler.h"
#include "output.h"

struct shared_data;


class InputMangler : public QObject
{
	Q_OBJECT
public:
	InputMangler();
	virtual ~InputMangler();
	//QString getThatStupidWindowTitleFromX(Window* window);

public slots:
	void cleanUp();
	void activeWindowChanged(QString wclass, QString title);
	void activeWindowTitleChanged(QString title);
	void reReadConfig();
	void printWinInfo();
	void printConfig();
	
private:
	QList<AbstractInputHandler*> handlers;
	QString wm_class, wm_title;
	QMap<QString, TransformationStructure> wsets;
	bool readConf();
	bool hasSettingsForId(QString id, QDomElement element);
	QVector<OutEvent> parseOutputs(QString id, QDomElement element, QVector<OutEvent> def);
	QVector<OutEvent> parseOutputsShort(QString confString);
	QMultiMap<QString, AbstractInputHandler*> handlersById;
// 	void stopHandlers();
	
};


