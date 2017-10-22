/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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

#include "macropartbase.h"
#include "outmacropart.h"
#include "outwait.h"
#include <QDebug>

MacroPartBase *MacroPartBase::parseMacro(QStringList &macroParts, __u16 sourceType) {
    if (macroParts.isEmpty())
        return nullptr;

    QString s = macroParts.first();
    if (s.startsWith('~')) {
        if (s.length() < 2) {
            qDebug() << ("'~' without followup in Macro configuration " + macroParts.join(", "));
            return nullptr;
        } else if (s.at(1) == 's') {
            return new OutWait(macroParts, sourceType);
        } else {
            qDebug() << ("unknown directive \"" + s + "\" in Macro configuration " + macroParts.join(", "));
            return nullptr;
        }
    }

    return new OutMacroPart(macroParts, sourceType);
}

void MacroPartBase::setInputBits(QBitArray **inputBits) {
    if (next != nullptr)
        next->setInputBits(inputBits);
};


