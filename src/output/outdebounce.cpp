/*
* Created by arek on 10.10.17.
*/

#include "outdebounce.h"
#include "keydefs.h"

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
