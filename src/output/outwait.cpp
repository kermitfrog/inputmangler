/*
* Created by arek on 07.10.17.
*/

#include "outwait.h"

OutWait::OutWait(QStringList &macroParts, __u16 sourceType) {
    QString s = macroParts.takeFirst();
    time = s.midRef(2).toInt();
    next = parseMacro(macroParts, sourceType);
}

void OutWait::proceed() {
    usleep((__useconds_t) time); //TODO: make it non-blocking!
    if (next != nullptr)
        next->proceed();
}

__u16 OutWait::getSourceType() const {
    if (next != nullptr)
        return next->getSourceType();
    return 0;
}


