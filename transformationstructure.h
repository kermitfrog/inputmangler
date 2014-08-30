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

#include "output.h"

/*!
 * @brief WindowSettings contains the Output settings for one window class.
 * 1/Window class in TransformationStructure, e.g:
 * <window class="Konsole" F="S,n+C,R,B" M="LEFT+S,RIGHT+S"/> (no title-matching)
 * <window class="Opera" F="S,_,R,B">  (with title-matching)
 *     <title regex="Dude and Zombies.*" F="S,ESC,BTN_LEFT,_"/>
 * <window/>
 */
class WindowSettings 
{
	friend class TransformationStructure;
public:
	WindowSettings(){};
	~WindowSettings();
	QVector<OutEvent> def;                //!< value when no title matches
	QVector<QRegularExpression*> titles;  //!< list of title regexes
	QVector< QVector<OutEvent> > events;  //!< events are matched to titles via index
};

/*!
 * @brief Holds all window-specific settings for a given id.
 */
class TransformationStructure 
{
public:
	TransformationStructure(){};
	~TransformationStructure();
	QVector<OutEvent> getOutputs(QString window_class, QString window_name);
	WindowSettings *window(QString w, bool create = false);
	bool sanityCheck(int numInputs, QString id, bool verbose = false);
	QVector<OutEvent> def; //!< Default outputs for id.
protected:
	QHash<QString, WindowSettings*> classes; //!< HashMap of window classes.
};