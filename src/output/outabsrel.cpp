/*
* Created by arek on 21.05.19.
*/

#include "outabsrel.h"
#include "../../shared/keydefs.h"

__u16 OutAbsRel::getSourceType() const {
    return 0;
}

OutAbsRel::OutAbsRel(QStringList l, __u16 sourceType) {
    if (l.size() != 2) {
        invalidate("OutAbsRel: needs exactly 2 Arguments");
        srcdst = OTHER;
        return;
    }
    InputEvent e = keymap[l[0]];

    if (sourceType != EV_ABS && sourceType != EV_ABSJ && e.type != EV_REL) {
        invalidate("OutAbsRel only works for ABS -> REL");
        srcdst = OTHER;
        return;
    }
    srcdst = ABS__REL;
    registerEvent();

    eventsSize = 2 * sizeof(input_event);
    event.eventChain = new input_event[2];
    e.setInputEvent(event.eventChain, 0);
    setSync(event.eventChain[1]);

    this->factor = l[1].toFloat();
//    this->center = center;
    currentPos = 0;
//    min = e.absmin;
//    max = e.absmax;
}

bool OutAbsRel::isValid() {
    return srcdst == ABS__REL;
}

OutAbsRel::~OutAbsRel() {
    if (isValid())
        delete event.eventChain;
}

void OutAbsRel::send(const __s32 &value, const timeval &time) {
    __s32 target = value * valueMod;
    if (target == currentPos)
        return;

    event.eventChain[0].value = target - currentPos;
    write(fds[fdnum], event.eventChain,
          eventsSize); // TODO valgrind says something about "points to uninitialized byte(s)" no idea yet
}

