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

OutMacroStart::OutMacroStart(QStringList l, __u16 sourceType) {
    qDebug() << "Parsing OutMacroStart: sourceType=" << sourceType << ", l=" << l;
    QStringList press, repeat, release;
    for (int i = 0; i < l.size(); ++i)
        l[i] = l[i].trimmed();

    if (sourceType == EV_KEY)
        srcdst = KEY__;
    else
        srcdst = OTHER;

    switch (l.count(MacroValDevider)) {
        case 0:
            press = l;
            break;
        case 1:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            release = l;
            break;
        case 2:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            while (l.at(0) != MacroValDevider)
                repeat.append(l.takeFirst());
            l.removeFirst();
            release = l;
            break;
        default:
            invalidate("too many \"" + QString(MacroValDevider) + "\" in " + l.join(", "));
    }
    
    qDebug() << "Parsing OutMacroStart: l=" << l << ", press=" << press << ", release=" << release << ", repeat=" << repeat;
    qDebug() << "Parsing OutMacroStart: sizes: press=" << press.size() << ", release=" << release.size() << ", repeat=" << repeat.size();

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
    qDebug() << "Start with value=" << value << ", srcdst=" << srcdst;
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
