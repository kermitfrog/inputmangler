/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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

#include "outdebounce.h"
#include "../../shared/keydefs.h"

OutDebounce::OutDebounce(QStringList l, __u16 sourceType) {
    if (l.count() < 1)
        return;
    init(keymap[l.at(0)], sourceType);
    if (l.count() == 2)
        delay = l.at(1).toInt();
    else
        delay = 150;
}

void OutDebounce::send(const __s32 &value, const timeval &time) {

    if ( (timeDiff(time) >= delay && value != 0 )
         || value == 0 && lastValue != 0 ) {
        OutSimple::send(value, time);
        lastValue = value;
    }
    lastTime.tv_sec = time.tv_sec;
    lastTime.tv_usec = time.tv_usec;
}

long OutDebounce::timeDiff(const timeval &newTime) {
    return (newTime.tv_sec - lastTime.tv_sec) * 1000 + (newTime.tv_usec - lastTime.tv_usec) / 1000;
}

QString OutDebounce::toString() const {
    return "~D(" + OutSimple::toString() + ", " + QString::number(delay) + "~)";
}

