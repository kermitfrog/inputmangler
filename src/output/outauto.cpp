/*
* Created by arek on 10.10.17.
*/

#include "outauto.h"
#include "keydefs.h"

OutAuto::OutAuto(QStringList l, __u16 sourceType) {
    if (l.count() != 1) {
        eventsSize = 0;
        return;
    }
    InputEvent e = keymap[l.at(0)];

    if (sourceType == EV_KEY)
        if (e.type == EV_KEY) {
            event.eventChains = new input_event *[3];
            srcdst = KEY__KEY;
            eventsSize = 2 * sizeof(input_event);

            event.eventChains[0] = new input_event[2];
            e.setInputEvent(event.eventChains[0], 0);
            setSync(event.eventChains[0][1]);

            event.eventChains[1] = new input_event[2];
            e.setInputEvent(event.eventChains[1], 1);
            setSync(event.eventChains[1][1]);

            event.eventChains[2] = new input_event[3];
            e.setInputEvent(event.eventChains[2], 1);
            e.setInputEvent(&event.eventChains[2][1], 0);
            setSync(event.eventChains[2][2]);

            fdnum = e.getFd();
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

