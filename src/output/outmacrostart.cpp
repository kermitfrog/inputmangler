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

#include "outmacrostart.h"
#include "macropartbase.h"

/**
 * Constructs a Macro.
 * Macros consist of a series of events.
 * OutMacroStart can have up to 3 starting points (press, repeat, release). If the source is not a key event, only the
 * press part is used internaly.
 *
 * @param l list of up to 3 lists of events as QStrings.
 * @param sourceType type of the source, triggering this event
 */
OutMacroStart::OutMacroStart(QList<QStringList> l, __u16 sourceType) {
    registerEvent();
    QStringList press, repeat, release;
    for (int i = 0; i < l.size(); ++i)
        for (int j = 0; j < l[i].size(); ++j)
            l[i][j] = l[i][j].trimmed();

    if (sourceType == EV_KEY)
        srcdst = KEY__;
    else
        srcdst = OTHER;

    press = l[0];

    switch (l.count()) {
        case 1:
            break;
        case 2:
            release = l[1];
            break;
        case 3:
            release = l[2];
            repeat = l[1];
            break;
        default:
            invalidate("too many \"" + QString(MacroValDevider)); // " + QString(l);
    }
    
    // this shouldn't be configured this way... but in case of copy & paste from a KEY__ to an OTHER source type,
    // we just want to ignore repeat events and send release immediately after press
    if (srcdst == OTHER) {
        press += release;
        release.clear();
        repeat.clear();
    }
    
    if (release.size() == 0 && repeat.size() == 0) {
        if (press.size() > 0) {
            macroParts[1] = MacroPartBase::parseMacro(press, sourceType);
            macroParts[0] = nullptr;
            macroParts[2] = nullptr;
        }
    } else {
        if (press.size() > 0)
            macroParts[1] = MacroPartBase::parseMacro(press, sourceType);
        else
            macroParts[1] = nullptr;

        if (release.size() > 0)
            macroParts[0] = MacroPartBase::parseMacro(release, sourceType);
        else
            macroParts[0] = nullptr;

        if (repeat.size() > 0)
            macroParts[2] = MacroPartBase::parseMacro(repeat, sourceType);
        else
            macroParts[2] = nullptr;
    }
}

void OutMacroStart::send(const __s32 &value, const timeval &time) {
    switch (srcdst) {
        case KEY__:
            if (macroParts[value] != nullptr)
                macroParts[value]->proceed();
            break;
        case OTHER:
            macroParts[1]->proceed();
            break;
        default:
            break;
    }


}

__u16 OutMacroStart::getSourceType() const {
    return macroParts[1]->getSourceType();
}

void OutMacroStart::setInputBits(QBitArray **inputBits) {
    switch (srcdst) {
        case KEY__:
            for(int i = 0; i < 3; ++i)
                if (macroParts[i] != nullptr)
                macroParts[i]->setInputBits(inputBits);
            break;
        case OTHER:
            macroParts[1]->setInputBits(inputBits);
            break;
        default:
            break;
    }
}

OutMacroStart::~OutMacroStart() {
    for (int i = 0; i < 3; ++i)
        if (macroParts[i] != nullptr)
            delete macroParts[i];
}

QString OutMacroStart::toString() const {
    QString r = "~Seq(";
    if (macroParts[1] != nullptr)
        r += macroParts[1]->toString();
    if (macroParts[2] != nullptr)
        r += " ~| " + macroParts[2]->toString();
    if (macroParts[0] != nullptr)
        r += " ~| " + macroParts[0]->toString();
    r += "~)";
    return r;
}
