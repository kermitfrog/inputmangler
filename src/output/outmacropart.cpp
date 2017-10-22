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

#include "outmacropart.h"
#include "../keydefs.h"

OutMacroPart::OutMacroPart(QStringList &macroParts, __u16 sourceType) {

    InputEvent ie;
    QList<InputEvent> ies;
    QList<__s32> values;
    QStringList mPart = macroParts.takeFirst().split(' ', QString::SkipEmptyParts);
    if (mPart.count() != 2) {
        invalidate("Something is wrong at start of \"" + macroParts.join(", ") + "\"");
        return;
    }

    ies.append(keymap[mPart.at(0)]);
    values.append(
            (__s32) mPart.last().toLong()); // TODO is there some preproccessor directive to make sure this converts to __s32?

    __u8 dtype = ies.at(0).getFd(); // TODO what about Joysticks? make sure this works!
    fdnum = (__u8) dtype; // TODO ABSJ ?

    while (!macroParts.isEmpty()) {
        if (macroParts.first().startsWith("~"))
            break;
        QStringList mPart = macroParts.first().split(' ', QString::SkipEmptyParts);
        if (mPart.count() != 2) {
            invalidate("Something is wrong at start of \"" + macroParts.join(", ") + "\"");
            return;
        }
        ie = keymap[mPart.first()];
        if (ie.getFd() == dtype) {
            ies.append(ie);
            values.append((__s32) mPart.last().toLong());
            macroParts.removeFirst();
        } else
            break;
    }

    event.eventChain = new input_event[ies.count() + 1];
    eventsSize = (ies.count() + 1) * sizeof(input_event);

    int i = 0;
    while (!ies.isEmpty()) {
        ie = ies.takeFirst();
        ie.setInputEvent(&event.eventChain[i], values.takeFirst());
        ++i;
    }
    setSync(event.eventChain[i]);

    // TODO do we need this?
    setSrcDst(sourceType, event.eventChain[0].type);

    if (srcdst == 0)
        invalidate("Invalid input/output type combination");
    else if (!macroParts.isEmpty()) {
        next = parseMacro(macroParts, sourceType);
        if (!next->isValid())
            invalidate();
    }

}

void OutMacroPart::proceed() {
    qDebug() << "Type is " << srcdst;
    write(fds[fdnum], event.eventChain, eventsSize);
    for (int i = 0; i < eventsSize / sizeof(input_event); ++i)
        qDebug() << "In MacroPart, sending event: type=" << event.eventChain[i].type << ", code="
                 << event.eventChain[i].code << ", value=" << event.eventChain[i].value;
    if (next != nullptr)
        next->proceed();
}

__u16 OutMacroPart::getSourceType() const {
    return (srcdst & SrcDst::INMASK) / 0b100;
}

void OutMacroPart::setInputBits(QBitArray **inputBits) {
    MacroPartBase::setInputBits(inputBits);

    if (eventsSize == 0)
        return;

    __u16 evType, code;
    int num = (int) eventsSize / sizeof(input_event) - 1;
    evType = event.eventChain[0].type;
    for (int i = 0; i < num; ++i) {
        code = event.eventChain[i].code;
        inputBits[EV_CNT]->setBit(evType);
        if (fdnum == EV_ABSJ && evType == EV_ABS)
            inputBits[EV_ABSJ]->setBit(code);
        else
            inputBits[evType]->setBit(code);
    }

}
