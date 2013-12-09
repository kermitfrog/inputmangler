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
#include "keydefs.h"

class AbstractInputHandler;

#include <qobject.h>
#include "abstractinputhandler.h"
#include <QtDBus/QtDBus>
#include <X11/Xutil.h>
#include <linux/input.h> //__u16...

struct shared_data;

// structure with information on input devices as read
// from /proc/bus/input/devices
class idevs 
{
public:
	QString vendor;
	QString product;
	QString event;
	QString id;
	bool mouse;
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
	//QString getThatStupidWindowTitleFromX(Window* window);

public slots:
	void cleanUp();
	void activeWindowChanged(QString w);
	void activeWindowTitleChanged(QString w);
	void reReadConfig();
	
private:
	shared_data * sd;
	QList<AbstractInputHandler*> handlers;
	QList<idevs> parseInputDevices();
	Display *display;
	QString wm_class, wm_title;
	QMap<QString, TransformationStructure> wsets;
	bool readConf();
// 	void stopHandlers();
	
};

/*
 * an output event, e.g: 
 * "S" for press shift
 * "g" for press g
 * "d+C" for press Ctrl-D
 */
class OutEvent 
{
public:
	OutEvent() {};
	OutEvent(int c) {keycode = c;};
	OutEvent(QString s);
	OutEvent(__s32 code, bool shift, bool alt = false, bool ctrl = false); 
	QString print()
	{
		QString s = QString::number(keycode) + " [";
		for(int i = 0; i < modifiers.count(); i++)
			s += QString::number(modifiers[i]) + ", ";
		return s + "]";
	};
	QVector<__u16> modifiers;
	__u16 keycode;
	__u16 code() const {return keycode;};
#ifdef DEBUGME
	QString initString;
#endif
	
};

/*
 * 1/Window class in TransformationStructure, e.g:
 * <window class="Konsole" F="S,n+C,R,B" M="LEFT+S,RIGHT+S"/> (no title-matching)
 * <window class="Opera" F="S,_,R,B">  (with title-matching)
 *     <title regex="Dude and Zombies.*" F="S,ESC,BTN_LEFT,_"/>
 * <window/>
 */
class WindowSettings 
{
public:
	WindowSettings(){};
	~WindowSettings();
	QVector<OutEvent> def;                // value when no title matches
	QVector<QRegularExpression*> titles;  // list of title regexes
	QVector< QVector<OutEvent> > events;  // events are matched to titles via index
};

/*
 * 1/id - holds all window-specific settings for a given id
 */
class TransformationStructure 
{
public:
	TransformationStructure(){};
	~TransformationStructure();
	QVector<OutEvent> getOutputs(QString window_class, QString window_name);
	WindowSettings *window(QString w, bool create = false);
	bool sanityCheck(int s, QString id);
	QVector<OutEvent> def;
	QHash<QString, WindowSettings*> classes;
};

#endif // INPUTMANGLER_H
