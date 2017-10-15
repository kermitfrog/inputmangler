/*
* Created by arek on 06.10.17.
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

    __u16 dtype = ies.at(0).type; // TODO what about Joysticks? make sure this works!
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
        if (ie.type == dtype) {
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
    setSrcDst(sourceType, dtype);

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
    for (int i = 0; i < eventsSize/ sizeof(input_event); ++i)
        qDebug() << "In MacroPart, sending event: type=" << event.eventChain[i].type << ", code=" << event.eventChain[i].code << ", value=" << event.eventChain[i].value;
    if (next != nullptr)
        next->proceed();
}

__u16 OutMacroPart::getSourceType() const {
    return (srcdst & SrcDst::INMASK) / 0b100;
}
