/*
* Created by arek on 10.10.17.
*/

#include "../keydefs.h"
#include "outaccel.h"


OutAccel::OutAccel(QStringList params, __u16 sourceType) {
    InputEvent ie = keymap[params[0].trimmed()];
    setSrcDst(sourceType, ie.type);

    if ( (srcdst & 0b1100) == 0b1100 || (srcdst & 0b0011) == 0b0011) {
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
        accelRate = 2.25;

    if (params.size() > 2)
        max = params[2].toFloat();
    else
        max = 15.0;

    if (params.size() > 3)
        maxDelay = params[3].toInt();
    else
        maxDelay = 400;

    if (params.size() > 4)
        minKeyPresses = params[4].toInt();
    else
        minKeyPresses = 2;

    triggered = 0;
    fdnum = (__u8) ie.type;
}

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
        qDebug() << "Nope";
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
    OutEvent::setInputBits(inputBits);
}

long int OutAccel::timeDiff(const timeval &newTime) {
    return (newTime.tv_sec - lastTime.tv_sec) * 1000 + (newTime.tv_usec - lastTime.tv_usec) / 1000;
}
