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
#include "keydefs.h"

class AbstractInputHandler;

#include <qobject.h>
#include <QtDBus/QtDBus>
#include <QtXml>
#include "abstractinputhandler.h"
#include "transformationstructure.h"

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
	QList<AbstractInputHandler*> handlers; //!< List of all handlers.
	QString wm_class, wm_title; //!< Current window class and title.
	QMap<QString, TransformationStructure> wsets; //!< Map containing all window specific outputs for all ids
	bool readConf();
    QVector<OutEvent> parseOutputsShort(QString s);
	QVector<OutEvent> parseOutputsLong(QXmlStreamReader &conf, QVector<OutEvent> def);
	QMultiMap<QString, AbstractInputHandler*> handlersById;
	bool readWindowSettings(QXmlStreamReader &conf, QStringList &ids);
// 	void stopHandlers();
	
protected:
    void xmlError(QXmlStreamReader &conf);
};


