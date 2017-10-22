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
#include "definitions.h"
#include <QBitArray>

class ConfParser
{
public:
    ConfParser(QList<AbstractInputHandler*> *_handlers, QMap<QString, TransformationStructure> *_wsets);
    QBitArray evbits, keybits, ledbits, relbits, absbitsT, absbitsJ, mscbits, synbits;
    QBitArray** getInputBits() { return inputBits;};

private:
    QList<AbstractInputHandler*> *handlers; //!< List of all handlers.
    QMap<QString, TransformationStructure> *wsets; //!< Map containing all window specific outputs for all ids

    QStringList ids;
    bool readConf();
    QVector<OutEvent*> parseOutputsShort(const QString str, QVector<OutEvent*> &vector);
    QMap<QString, AbstractInputHandler*> handlersById; // different than in inputmangler.cpp - needs only 1 per id
    void readWindowSettings(pugi::xml_node window, QMap<QString, QVector<OutEvent*>> defaultOutputs, QMap<QString, bool> usedIds);

protected:
    void parseWindowSettings(pugi::xml_node group, QMap<QString, QVector<OutEvent*>> defaultOutputs, QMap<QString, bool> usedIds);

    QVector<OutEvent*> parseOutputsLong(pugi::xml_node node, const AbstractInputHandler * handler, QVector<OutEvent*> def);
    
    // Bits for uinput device setup
    QBitArray* inputBits[NUM_INPUTBITS];

};



#endif //INPUTMANGLER_CONFPARSER_H
