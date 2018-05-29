/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2016-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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


#ifndef INPUTMANGLER_CONFPARSER_H
#define INPUTMANGLER_CONFPARSER_H

#include <handlers/abstractinputhandler.h>
#include <pugixml.hpp>
#include <linux/input-event-codes.h>
#include "../shared/definitions.h"
#include <QBitArray>

/*!
 * @brief Parses configuration file.
 */
class ConfParser
{
public:
    ConfParser(QList<AbstractInputHandler*> *_handlers, QMap<QString, TransformationStructure> *_wsets);
    ~ConfParser();
    QBitArray evbits;   //!< Stores flags for uinput configuration: Event types (Key, Relative Movement, ...)
    QBitArray keybits;  //!< Stores flags for uinput configuration: Keys
    QBitArray ledbits;  //!< Stores flags for uinput configuration: Leds (not used yet)
    QBitArray relbits;  //!< Stores flags for uinput configuration: Relative Movements (Mouse Wheel, Mouse X Axis, ...)
    QBitArray absbitsT; //!< Stores flags for uinput configuration: Absolute Movement on Tablet (Pen Position on X, ...)
    QBitArray absbitsJ; //!< Stores flags for uinput configuration: Absolute Movement on Joystick (Position on X, ...)
    QBitArray mscbits;  //!< Stores flags for uinput configuration: Misc events
    QBitArray synbits;  //!< Stores flags for uinput configuration: Sychronization events
    QBitArray** getInputBits() { return inputBits;};

private:
    QList<AbstractInputHandler*> *handlers; //!< List of all handlers.
    QMap<QString, TransformationStructure> *wsets; //!< Map containing all window specific outputs for all ids

    QStringList ids; //!< List of handler ids (as configured in <device [..] id="ID" > )
    bool readConf();
    QVector<OutEvent*> parseOutputsShort(const QString &str, QVector<OutEvent*> &vector);
    QMap<QString, AbstractInputHandler*> handlersById; //!< different than in inputmangler.cpp - needs only 1 per id
    void readWindowSettings(pugi::xml_node window, QMap<QString, QVector<OutEvent*>> defaultOutputs, QMap<QString, bool> usedIds);

protected:
    void parseWindowSettings(pugi::xml_node group, QMap<QString, QVector<OutEvent*>> defaultOutputs, QMap<QString, bool> usedIds);

    QVector<OutEvent*> parseOutputsLong(pugi::xml_node node, const AbstractInputHandler * handler, QVector<OutEvent*> def);

    QBitArray* inputBits[NUM_INPUTBITS]; //!< Bits for uinput device setup (Array storing pointers to *bits)
    QStringList parseSplit(const QString &str) const;
    QRegExp specialSeq;
    QRegExp noWS = QRegExp("\\S");
};



#endif //INPUTMANGLER_CONFPARSER_H
