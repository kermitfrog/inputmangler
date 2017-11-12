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

#include "../keydefs.h"
#include "outaccel.h"

/**
 *
 * @param params by order: AccelRate      By how much it will get faster each time. Floating point value.
           AccelMax       It won't get faster than that. Floating point value.
           MaxDelay       Milliseconds until Acceleration will reset. Integer.
           minKeyPresses  Get faster after so many scroll steps. Integer.
 * @param sourceType
 */
OutAccel::OutAccel(QStringList params, __u16 sourceType) {
    registerEvent();
    InputEvent ie = keymap[params[0].trimmed()];
    setSrcDst(sourceType, ie.type);

    if ((srcdst & 0b1100) == 0b1100 || (srcdst & 0b0011) == 0b0011) {
        invalidate(
                "OutAccel: unsupported translation " + QString::number(sourceType) + " => " + QString::number(ie.type)
                + " in " + params.join(", "));
        return;
    }

    normValue = (ie.type == EV_REL && ie.valueType == Negative) ? -1 : 1;
    event.eventChain = new input_event[2];
    ie.setInputEvent(&event.eventChain[0], normValue);
    setSync(event.eventChain[1]);
    eventsSize = sizeof(input_event) * 2;


    qDebug() << params;
    if (params.size() > 1)
        accelRate = params[1].toFloat();
    else
        accelRate = AccelRateDefault;

    if (params.size() > 2)
        max = params[2].toFloat();
    else
        max = MaxDefault;

    if (params.size() > 3)
        maxDelay = params[3].toInt();
    else
        maxDelay = MaxDelayDefault;

    if (params.size() > 4)
        minKeyPresses = params[4].toInt();
    else
        minKeyPresses = MinKeyPressDefault;

    triggered = 0;
    fdnum = ie.getFd();
}

// TODO improve algorithm, then write documentation
void OutAccel::send(const __s32 &value, const timeval &time) {
    if (value == 0)
        return;

    if (timeDiff(time) > maxDelay) {
        // reset
        triggered = 0;
        currentRate = 1.0;
        overhead = 0.0;
        event.eventChain->value = normValue;
        write(fds[fdnum], event.eventChain, eventsSize);
    } else
        switch (srcdst) {
            case KEY__REL:
                if (value == 2)
                    return;
            case REL__REL:
                // mouse wheel or movement
                triggered++; // times triggered
                if (triggered >= minKeyPresses) {
                    currentRate += accelRate * value * normValue;
                    if (currentRate > max)
                        currentRate = max;
                    // TODO does this work in *all* applications???
                    event.eventChain->value = normValue * (int) (currentRate + 0.5);
                    write(fds[fdnum], event.eventChain, eventsSize);
                    qDebug() << "Value = " << event.eventChain->value;
                }
                break;
            default:
                overhead += currentRate;
                write(fds[fdnum], event.eventChain, eventsSize);
                while (overhead > 1.0) {
                    usleep(2000);
                    event.eventChain->value = 0;
                    write(fds[fdnum], event.eventChain, eventsSize);
                    usleep(2000);  //
                    event.eventChain->value = normValue;
                    write(fds[fdnum], event.eventChain, eventsSize);
                    overhead -= 1.0;
                }
        }

    lastTime.tv_sec = time.tv_sec;
    lastTime.tv_usec = time.tv_usec;

}

void OutAccel::setInputBits(QBitArray **inputBits) {
    __u16 evType, code;
    evType = event.eventChain[0].type;
    code = event.eventChain[0].code;

    inputBits[EV_CNT]->setBit(evType);
    if (fdnum == EV_ABSJ && evType == EV_ABS)
        inputBits[EV_ABSJ]->setBit(code);
    else
        inputBits[evType]->setBit(code);
}


long int OutAccel::timeDiff(const timeval &newTime) {
    return (newTime.tv_sec - lastTime.tv_sec) * 1000 + (newTime.tv_usec - lastTime.tv_usec) / 1000;
}

QString OutAccel::toString() const {
    QString r = "~+(";
    if (eventsSize == 0)
        return "invalid";
    r = getStringForCode(event.eventChain->code, event.eventChain->type, fdnum);
    r += ", " + QString::number(accelRate);
    r += ", " + QString::number(max);
    r += ", " + QString::number(maxDelay);
    r += ", " + QString::number(minKeyPresses);
    r += "~)";
    return r;
}
