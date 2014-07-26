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

#include <linux/input.h> //__u16...
#include <QString>
#include <QVector>
#include <QHash>



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
	QString print() const;
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
	bool sanityCheck(int s, QString id, bool debug = false);
	QVector<OutEvent> def;
	QHash<QString, WindowSettings*> classes;
};

