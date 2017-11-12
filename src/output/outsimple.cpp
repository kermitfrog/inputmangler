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

#include <keydefs.h>
#include "outsimple.h"

/**
 * @param e base InputEvent
 * @param sourceType type of the event that triggers this OutEvent (as configured in <signal key= >)
 */
OutSimple::OutSimple(InputEvent &e, __u16 sourceType) {
    init(e, sourceType);
}

/*!
 * @brief send an event; usualy called by handler;
 *
 * @param value value from input
 * @param time time the input event was generated by the kernel
 */
void OutSimple::send(const __s32 &value, const timeval &time) {
    // send a predefined event, based on source and destination type.
    // see init(const __s32 &value, const timeval &time) for more explanations on this
    switch (srcdst) {
        case KEY__KEY:
            write(fds[fdnum], event.eventChains[value], eventsSize);
            break;
        case KEY__REL:
            if (value != 0)
                write(fds[fdnum], event.eventChain, eventsSize);
            break;
        case REL__KEY:
            write(fds[fdnum], event.eventChain, eventsSize);
            break;
        case REL__REL:
            event.eventChain[0].value = value * valueMod;
            write(fds[fdnum], event.eventChain, eventsSize); // TODO valgrind says something about "points to uninitialized byte(s)" no idea yet
            break;
        case ABS__ABS:
            event.eventChain[0].value = value;
            write(fds[fdnum], event.eventChain, eventsSize);
            break;
        default:
            break;
    }

}

/**
 * free some memory
 */
OutSimple::~OutSimple() {
    switch (srcdst) {
        case KEY__KEY:
            delete[] event.eventChains[2];
            delete[] event.eventChains[1];
            delete[] event.eventChains[0];
            delete[] event.eventChains;
            break;
        case ABS__ABS:
        case KEY__REL:
        case REL__KEY:
        case REL__REL:
            delete[] event.eventChain;
            break;
        default:
            break;
    }

}

/**
 * returns type of source event
 */
__u16 OutSimple::getSourceType() const {
    __u16 t = (srcdst & SrcDst::INMASK) / 0b100;
    if (valueMod < 0)
        t += NegativeModifier;
    return t;
}

/**
 * @brief
 * @param e
 * @param sourceType
 */
void OutSimple::init(InputEvent &e, __u16 sourceType) {
    registerEvent();
    interpretNegSource(sourceType);
    // as a general rule, events are pregenerated as far as feasible
    switch (sourceType) {
        case EV_KEY:
            switch (e.type) {
                case EV_KEY:
                    /**
                     * KEY__KEY
                     * only type that makes use of event.eventChains, as every possible value
                     * (0: release, 1: press, 2: repeat) has exactly one pregeneratable translation.
                     * so we can simply send it with a write of event.eventChains[value]
                     */
                    event.eventChains = new input_event *[3];
                    srcdst = KEY__KEY;
                    eventsSize = 2 * sizeof(input_event); // 2 = event + sync
                    for (int i = 0; i < 3; ++i) {
                        event.eventChains[i] = new input_event[2];
                        e.setInputEvent(event.eventChains[i], i);
                        // every write to uinput should be finished by a sync event
                        setSync(event.eventChains[i][1]);
                    }
                    break;
                case EV_REL:
                    /**
                     * all other srcdst types pregenerate one event and thus use event.eventChain
                     */
                    srcdst = KEY__REL;
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    // A key will generate a press of either the positive or negetive movement
                    e.setInputEvent(event.eventChain, e.valueType == Negative ? -1 : 1);
                    setSync(event.eventChain[1]);
                    break;
                case EV_ABS:
                    // not implemented
                    // possible use: Keyboard to Joystick Axis
                    eventsSize = 0;
                    srcdst = KEY__ABS;
                    break;
            };
            break;
        case EV_REL:
            switch (e.type) {
                case EV_KEY:
                    /**
                     * REL events can have positive or negative values (e.g. left or right on an
                     * axis) there is no release, so for keys we always generate a press, followed
                     * by a release (and a sync).
                     * If the event should only be triggered on + or -, the handler has to take care
                     * of that (and use sendRaw() otherwise).
                     * Mice rarely send something other than 1|-1 anyway, so we don't care about the
                     * exact value. Doing that probably would offer no real benefit anyway.
                     */
                    srcdst = REL__KEY;
                    eventsSize = 3 * sizeof(input_event);
                    event.eventChain = new input_event[3];
                    e.setInputEvent(&event.eventChain[0], 1);
                    e.setInputEvent(&event.eventChain[1], 0);
                    setSync(event.eventChain[2]);
                    break;
                case EV_REL:
                    srcdst = REL__REL;
                    /**
                     * we set the value to 0, as it is changed to the (sometimes inverted) source
                     * value in send()
                     */
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    e.setInputEvent(event.eventChain, 0);
                    setSync(event.eventChain[1]);
                    break;
                case EV_ABS:
                    // not implemented
                    // possible use: no idea. Why would anyone do that??? maybe simulate a joystick with a mouse???
                    eventsSize = 0;
                    srcdst = REL__ABS;
                    break;
            };
            break;
        case EV_ABS:
            switch (e.type) {
                case EV_KEY:
                    /* not implemented
                       possible use: different event, depending on value. But this is out of scope
                       for this class */
                    eventsSize = 0;
                    srcdst = ABS__KEY;
                    break;
                case EV_REL:
                    /* not implemented
                       possible use: different event, depending on value.
                                     maybe translate to relative mouse movement
                       Both are out of scope for this class */
                    srcdst = ABS__REL;
                    eventsSize = 0;
                    break;
                case EV_ABS:
                    /**
                     * translate abs axis to another abs axis..
                     * this might require some extra work to be useful
                     */
                    srcdst = ABS__ABS;
                    eventsSize = 2 * sizeof(input_event);
                    event.eventChain = new input_event[2];
                    e.setInputEvent(event.eventChain, 0);
                    setSync(event.eventChain[1]);
                    break;
            };
            break;
    };

    fdnum = e.getFd();
}

QString OutSimple::toString() const {
    if (eventsSize == 0)
        return "invalid";
    if (srcdst == KEY__KEY)
        return getStringForCode(event.eventChains[1]->code, event.eventChains[1]->type, fdnum);
    return getStringForCode(event.eventChain->code, event.eventChain->type, fdnum);
}


