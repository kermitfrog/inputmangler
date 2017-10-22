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

#include "outwait.h"

OutWait::OutWait(QStringList &macroParts, __u16 sourceType) {
    QString s = macroParts.takeFirst();
    time = s.midRef(2).toInt();
    next = parseMacro(macroParts, sourceType);
}

void OutWait::proceed() {
    usleep((__useconds_t) time); //TODO: make it non-blocking!
    if (next != nullptr)
        next->proceed();
}

__u16 OutWait::getSourceType() const {
    if (next != nullptr)
        return next->getSourceType();
    return 0;
}


