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

#include "outsimple.h"

OutSimple::OutSimple(InputEvent &e, __u16 sourceType) {
    init(e, sourceType);
}

void OutSimple::send(const __s32 &value, const timeval &time) {
    switch (srcdst) {
        case KEY__KEY:
            write(fds[fdnum], event.eventChains[value], eventsSize);
            break;
        case KEY__REL:
            if (value != 0)
                write(fds[fdnum], event.eventChain, eventsSize);
            break;
        case REL__KEY: // TODO do we have to take care of +- here?
            write(fds[fdnum], event.eventChain, eventsSize);
            break;
        case REL__REL:
            event.eventChain[0].value = value;
            write(fds[fdnum], event.eventChain, eventsSize);
            break;
        case ABS__ABS:
            event.eventChain[0].value = value;
            write(fds[fdnum], event.eventChain, eventsSize);
            break;
        default:
            break;
    }

}

OutSimple::~OutSimple() {
    switch (srcdst) {
        case KEY__KEY:
            delete[] event.eventChains[2];
            delete[] event.eventChains[1];
        case ABS__ABS:
        case KEY__REL:
        case REL__KEY:
        case REL__REL:
            delete[] event.eventChains[0];
            delete[] event.eventChains;
            break;
        default:
            break;
    }

}

void OutSimple::setInputBits(QBitArray **inputBits) {

}

__u16 OutSimple::getSourceType() const {
    return (srcdst & SrcDst::INMASK) / 0b100;
}

void OutSimple::init(InputEvent &e, __u16 sourceType) {
    switch (sourceType) {
        case EV_KEY:
            switch (e.type) {
                case EV_KEY:
                    event.eventChains = new input_event *[3];
                    srcdst = KEY__KEY;
                    eventsSize = 2 * sizeof(input_event);
                    for (int i = 0; i < 3; ++i) {
                        event.eventChains[i] = new input_event[2];
                        e.setInputEvent(event.eventChains[i], i);
                        setSync(event.eventChains[i][1]);
                    }
                    break;
                case EV_REL:
                    srcdst = KEY__REL;
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    e.setInputEvent(event.eventChain, e.valueType == Negative ? -1 : 1);
                    setSync(event.eventChain[1]);
                    break;
                case EV_ABS:
                    eventsSize = 0;
                    srcdst = KEY__ABS;
                    break;
            };
            break;
        case EV_REL:
            switch (e.type) {
                case EV_KEY:
                    srcdst = REL__KEY;
                    eventsSize = 3 * sizeof(input_event);
                    event.eventChain = new input_event[3];
                    e.setInputEvent(&event.eventChain[0], 1);
                    e.setInputEvent(&event.eventChain[1], 2);
                    setSync(event.eventChain[2]);
                    break;
                case EV_REL:
                    srcdst = REL__REL;
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    e.setInputEvent(event.eventChain, e.valueType == Negative ? -1 : 1);
                    setSync(event.eventChain[1]);
                    break;
                case EV_ABS:
                    eventsSize = 0;
                    srcdst = REL__ABS;
                    break;
            };
            break;
        case EV_ABS:
            switch (e.type) {
                case EV_KEY:
                    eventsSize = 0;
                    srcdst = ABS__KEY;
                    break;
                case EV_REL:
                    srcdst = ABS__REL;
                    eventsSize = 0;
                    break;
                case EV_ABS:
                    srcdst = ABS__ABS;
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    e.setInputEvent(event.eventChain, 0);
                    setSync(event.eventChain[1]);
                    valueType = e.valueType;
                    break;
            };
            break;
    };

    fdnum = e.getFd();
}


