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

#include "outauto.h"
#include "keydefs.h"

OutAuto::OutAuto(QStringList l, __u16 sourceType) {
    if (l.count() != 1) {
        eventsSize = 0;
        registerEvent();
        return;
    }
    InputEvent e = keymap[l.at(0)];

    if (sourceType == EV_KEY && e.type == EV_KEY) {
        event.eventChains = new input_event *[3];
        srcdst = KEY__KEY;
        eventsSize = 2 * sizeof(input_event);

        event.eventChains[0] = new input_event[2];
        e.setInputEvent(event.eventChains[0], 0);
        setSync(event.eventChains[0][1]);

        event.eventChains[1] = new input_event[2];
        e.setInputEvent(event.eventChains[1], 1);
        setSync(event.eventChains[1][1]);

        // simply write press & release on a repeat
        event.eventChains[2] = new input_event[3];
        e.setInputEvent(event.eventChains[2], 1);
        e.setInputEvent(&event.eventChains[2][1], 0);
        setSync(event.eventChains[2][2]);

        fdnum = e.getFd();
        registerEvent();
        return;
    };

    init(e, sourceType);
}

void OutAuto::send(const __s32 &value, const timeval &time) {
    if (value == 2 && srcdst == KEY__KEY) {
        write(fds[fdnum], event.eventChains[2], sizeof(input_event) * 3);
        return;
    }

    OutSimple::send(value, time);
}

QString OutAuto::toString() const {
    return "~Auto(" + OutSimple::toString() + "~)";
}

