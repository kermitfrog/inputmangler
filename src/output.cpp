/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>

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

#include "output.h"
#include <fcntl.h>
#include <unistd.h>
#include <QList>
#include "keydefs.h"

int OutEvent::fds[5];

/*!
 * @brief opens virtual devices for output
 */
void OutEvent::generalSetup(QBitArray *inputBits[NUM_INPUTBITS]) {
    int fd;
    int err;
    bool need = false;
    uinput_user_dev *dev;

    fds[1] = fds[2] = fds[3] = fds[4] = 0;
    // keyboard
    // we always set up the keyboard with default modifiers, because checking if a modifier is used
    // somewhere is costly. also that check would most likely return true anyway
    inputBits[EV_KEY]->setBit(KEY_LEFTSHIFT);
    inputBits[EV_KEY]->setBit(KEY_LEFTALT);
    inputBits[EV_KEY]->setBit(KEY_LEFTCTRL);
    inputBits[EV_KEY]->setBit(KEY_LEFTMETA);
    inputBits[EV_KEY]->setBit(KEY_RIGHTALT);

    // do actual keyboard setup
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        qDebug() << "Unable to open /dev/uinput - Error: " << strerror(errno);
        exit(EXIT_FAILURE);
    }
    // UI_DEV_CREATE() is the new way - unfortunately it's problematic on current (as of 05/2017)
    // Ubuntu-LTS, as linux-libc-dev only provides headers for linux 4.4
    // also, the old way's documentation is less bad.
    err = ioctl(fd, UI_SET_EVBIT, EV_KEY) | ioctl(fd, UI_SET_EVBIT, EV_SYN);
    err = ioctl(fd, UI_SET_EVBIT, EV_MSC) | ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
    for (int i = 0; i < BTN_MISC; i++)
        if (inputBits[EV_KEY]->at(i))
            err |= ioctl(fd, UI_SET_KEYBIT, i);
    for (int i = KEY_OK; i < BTN_DPAD_UP; i++)
        if (inputBits[EV_KEY]->at(i))
            err |= ioctl(fd, UI_SET_KEYBIT, i);
    dev = makeUinputUserDev("Virtual Keyboard");
    err |= (write(fd, dev, sizeof(*dev)) < 0);
    err |= ioctl(fd, UI_DEV_CREATE);
    if (err) {
        qDebug() << "Error setting up virtual Keyboard!";
        exit(EXIT_FAILURE);
    }
    fds[1] = fd;

    // mouse
    need = false;
    if (inputBits[EV_CNT]->at(EV_KEY) || inputBits[EV_CNT]->at(EV_REL)) {
        for (int i = BTN_MISC; i < BTN_JOYSTICK; i++)
            if (inputBits[EV_KEY]->at(i)) {
                need = true;
                break;
            }
        if (!need)
            for (int i = REL_X; i < REL_CNT; i++)
                if (inputBits[EV_REL]->at(i)) {
                    need = true;
                    break;
                }
        if (need) { // do actual mouse setup
            fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
            if (fd < 0) {
                qDebug() << "Unable to open /dev/uinput - Error: " << fd;
                exit(EXIT_FAILURE);
            }
            err = ioctl(fd, UI_SET_EVBIT, EV_KEY) | ioctl(fd, UI_SET_EVBIT, EV_REL); // no need for SYN?
            err = ioctl(fd, UI_SET_EVBIT, EV_MSC) | ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
            for (int i = BTN_MISC; i < BTN_JOYSTICK; i++)
                if (inputBits[EV_KEY]->at(i))
                    err |= ioctl(fd, UI_SET_KEYBIT, i);
            for (int i = REL_X; i < REL_CNT; i++)
                if (inputBits[EV_REL]->at(i))
                    err |= ioctl(fd, UI_SET_RELBIT, i);
            dev = makeUinputUserDev("Virtual Mouse");
            err |= (write(fd, dev, sizeof(*dev)) < 0);
            err |= ioctl(fd, UI_DEV_CREATE);
            if (err) {
                qDebug() << "Error setting up virtual Mouse!";
                exit(EXIT_FAILURE);
            }
            fds[2] = fd;
        }
    }

    // tablet
    need = false;
    if (inputBits[EV_CNT]->at(EV_KEY) || inputBits[EV_CNT]->at(EV_ABS) || inputBits[EV_CNT]->at(EV_MSC)) {
        for (int i = BTN_DIGI; i < KEY_OK; i++)
            if (inputBits[EV_KEY]->at(i)) {
                need = true;
                break;
            }
        if (!need)
            for (int i = ABS_X; i < ABS_CNT; i++)
                if (inputBits[EV_ABS]->at(i)) {
                    need = true;
                    break;
                }
        if (need) { // do actual setup
            fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
            if (fd < 0) {
                qDebug() << "Unable to open /dev/uinput - Error: " << fd;
                exit(EXIT_FAILURE);
            }
            dev = makeUinputUserDev("Virtual Tablet");
            const int absBase = 10000 * EV_ABS + 1000 * ValueType::TabletAxis;
            err = ioctl(fd, UI_SET_EVBIT, EV_KEY) | ioctl(fd, UI_SET_EVBIT, EV_ABS)
                  | ioctl(fd, UI_SET_EVBIT, EV_SYN) | ioctl(fd, UI_SET_EVBIT, EV_MSC)
                  | ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
            for (int i = BTN_DIGI; i < KEY_OK; i++)
                if (inputBits[EV_KEY]->at(i))
                    err |= ioctl(fd, UI_SET_KEYBIT, i);
            for (int i = ABS_X; i < ABS_CNT; i++)
                if (inputBits[EV_ABS]->at(i)) {
                    err |= ioctl(fd, UI_SET_ABSBIT, i);
                    InputEvent ie = keymap[keymap_reverse[absBase + i]];
                    dev->absmin[i] = ie.absmin;
                    dev->absmax[i] = ie.absmax;
                }
            err |= (write(fd, dev, sizeof(*dev)) < 0);
            err |= ioctl(fd, UI_DEV_CREATE);
            if (err) {
                qDebug() << "Error setting up virtual Tablet!";
                exit(EXIT_FAILURE);
            }
            fds[3] = fd;
        }
    }

    // joystick / gamepad
    need = false;
    if (inputBits[EV_CNT]->at(EV_KEY) || inputBits[EV_CNT]->at(EV_ABS)) {
        for (int i = BTN_JOYSTICK; i < BTN_DIGI; i++)
            if (inputBits[EV_KEY]->at(i)) {
                need = true;
                break;
            }
        if (!need)
            for (int i = ABS_X; i < ABS_CNT; i++)
                if (inputBits[EV_ABSJ]->at(i)) {
                    need = true;
                    break;
                }
        if (need) { // do actual setup
            fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
            if (fd < 0) {
                qDebug() << "Unable to open /dev/uinput - Error: " << fd;
                exit(EXIT_FAILURE);
            }
            dev = makeUinputUserDev("Virtual Joystick/Gamepad");
            const int absBase = 10000 * EV_ABS + 1000 * ValueType::JoystickAxis;
            err = ioctl(fd, UI_SET_EVBIT, EV_KEY) | ioctl(fd, UI_SET_EVBIT, EV_ABS)
                  | ioctl(fd, UI_SET_EVBIT, EV_SYN);
            for (int i = BTN_JOYSTICK; i < BTN_DIGI; i++)
                if (inputBits[EV_KEY]->at(i))
                    err |= ioctl(fd, UI_SET_KEYBIT, i);
            for (int i = ABS_X; i < ABS_CNT; i++)
                if (inputBits[EV_ABSJ]->at(i)) {
                    err |= ioctl(fd, UI_SET_ABSBIT, i);
                    InputEvent ie = keymap[keymap_reverse[absBase + i]];
                    dev->absmin[i] = ie.absmin;
                    dev->absmax[i] = ie.absmax;
                }
            err |= (write(fd, dev, sizeof(*dev)) < 0);
            err |= ioctl(fd, UI_DEV_CREATE);
            if (err) {
                qDebug() << "Error setting up virtual Joystick!";
                exit(EXIT_FAILURE);
            }
            fds[4] = fd;
        }
    }

    /* before uinput
	openVDevice("/dev/virtual_kbd", 1);
	openVDevice("/dev/virtual_mouse", 2);
	openVDevice("/dev/virtual_tablet", 3);
	openVDevice("/dev/virtual_joystick", 4);
     */
#ifdef DEBUGME
    qDebug() << "kbd: " << OutEvent::fds[1] << ", mouse: " << OutEvent::fds[2];
    qDebug() << "tablet: " << OutEvent::fds[4] << ", joystick: " << OutEvent::fds[4];
#endif
}

uinput_user_dev *OutEvent::makeUinputUserDev(char *name) {
    uinput_user_dev *dev;
    dev = new uinput_user_dev;
    memset(dev, 0, sizeof(*dev));
    dev->id.bustype = BUS_USB;
    dev->id.vendor = dev->id.product = 0;
    dev->id.version = 1;
    snprintf(dev->name, UINPUT_MAX_NAME_SIZE, name);
    return dev;
}

/*!
 * @brief closes virtual devices
 */
void OutEvent::closeVirtualDevices() {
    for (int i = 1; i <= 4; i++) {
        ioctl(fds[i], UI_DEV_DESTROY);
        close(fds[i]);
    }
}


/*!
 * @brief  Constructs an output event from a config string, e.g "p+S"
 * possible values:
 *      Key
 *      Key+Mods  (Where each Modifier is exactly 1 char)
 *      ~Seq(Key Val, Key Val, ~sTime, [..], ~)
 *      ~+
 *      ~A | ~Auto
 *      ~D | ~Debounce
 */
OutEvent::OutEvent(QString s, __u16 sourceType) {
    s = s.trimmed();
    if (s.isEmpty())
        return;
    QStringList parts;
    if (s[0] != '~') {
        parts = s.split('+');
        if (parts.count() == 1)
            OutEvent(keymap[parts[0]], sourceType);
        else {
            QList<InputEvent> ies;
            parseMacro(comboToMacro(parts), sourceType);
        }


    }


}

OutEvent::~OutEvent() {
    switch (outType) {
        case Simple:
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
            }
            break;
        case MacroPart:
            delete[] event.eventChain;
            delete extra.next;
        case Wait:
            delete extra.next;
            break;
        case MacroStart:
            delete[] extra.macroParts;
            //delete extra.macroParts[1];
            //delete extra.macroParts[2];
            break;
            // TODO: other types
    }
}

QStringList OutEvent::comboToMacro(QStringList list) {
    QStringList macro;
    QString s;
    int i;

    s = list.at(1);
    for (i = 0; i < s.length(); ++i) {
        macro.append(QString(s.at(i)) + " 1");
    }
    macro.append(list.at(0) + " 1");
    macro.append(MacroValDevider);
    macro.append(list.at(0) + " 2");
    macro.append(MacroValDevider);
    macro.append(list.at(0) + " 0");


    for (i = s.length(); i >= 0; --i) {
        macro.append(QString(s.at(i)) + " 0");
    }

    return macro;
}

void OutEvent::parseMacro(QStringList l, __u16 sourceType) {
    QStringList press, repeat, release;
    for (int i = 0; i < l.count(); ++i)
        l[i] = l[i].trimmed();

    if (sourceType == EV_KEY)
        srcdst = KEY__;
    else
        srcdst = OTHER;

    switch (l.count(MacroValDevider)) {
        case 0:
            press = l;
            break;
        case 1:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            release = l;
        case 2:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            while (l.at(0) != MacroValDevider)
                repeat.append(l.takeFirst());
            l.removeFirst();
            release = l;
        default:
            // TODO set as invalid
            eventsSize = 0;
    }

    if (release.count() == repeat.count() == 0) {
        if (press.count() > 0)
            extra.next = new OutEvent((QStringList) press, sourceType);
    } else {
        MacroParts *mp = new MacroParts();
        if (press.count() > 0)
            mp->parts[1] = new OutEvent((QStringList) press, sourceType);
        else
            mp->parts[1] = nullptr;

        if (release.count() > 0)
            mp->parts[0] = new OutEvent((QStringList) release, sourceType);
        else
            mp->parts[0] = nullptr;

        if (repeat.count() > 0)
            mp->parts[2] = new OutEvent((QStringList) repeat, sourceType);
        else
            mp->parts[2] = nullptr;
    }

    outType = MacroStart;
}

OutEvent::OutEvent(QStringList &macroParts, __u16 sourceType) {
    if (macroParts.first().startsWith('~')) {
        QString s = macroParts.takeFirst();
        if (s.length() < 2) {
            invalidate("'~' without followup in Macro configuration " + macroParts.join(", "));
        } else if (s.at(1) == 's') {
            outType = Wait;
            customValue = s.midRef(2).toInt();
            if (!macroParts.isEmpty())
                extra.next = new OutEvent((QStringList) macroParts, sourceType);
        } else {
            invalidate("unknown directive \"" + s + "\" in Macro configuration " + macroParts.join(", "));
        }
        return;
    }

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

    outType = MacroPart;
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
    switch (sourceType) {
        case EV_KEY:
            switch (dtype) {
                case EV_KEY:
                    srcdst = KEY__KEY;
                    break;
                case EV_REL:
                    srcdst = KEY__REL;
                    break;
                case EV_ABS:
                    srcdst = KEY__ABS;
                    break;
            };
        case EV_REL:
            switch (dtype) {
                case EV_KEY:
                    srcdst = REL__KEY;
                    break;
                case EV_REL:
                    srcdst = REL__REL;
                    break;
                case EV_ABS:
                    srcdst = REL__ABS;
                    break;
            };
        case EV_ABS:
            switch (dtype) {
                case EV_KEY:
                    srcdst = ABS__KEY;
                    break;
                case EV_REL:
                    srcdst = ABS__REL;
                    break;
                case EV_ABS:
                    srcdst = ABS__ABS;
                    break;
            };
    };
    if (srcdst == 0)
        invalidate("Invalid input/output type combination");
    else if (!macroParts.isEmpty()) {
        extra.next = new OutEvent((QStringList) macroParts, sourceType);
        if (!extra.next->isValid())
            invalidate();
    }
}

OutEvent::OutEvent(InputEvent &e, __u16 sourceType) {
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
                    event.eventChains[0] = new input_event[2];
                    e.setInputEvent(event.eventChain, e.valueType == Negative ? -1 : 1);
                    setSync(event.eventChain[1]);
                    break;
                case EV_ABS:
                    eventsSize = 0;
                    srcdst = KEY__ABS;
                    break;
            };
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


    };
    fdnum = (__u8) e.type; // TODO ABSJ ?
}


void OutEvent::send(const __s32 &value, const timeval &time) {
    switch (outType) {
        case Simple:
            switch (srcdst) {
                case KEY__KEY:
                    write(fds[EV_KEY], event.eventChains[value], eventsSize);
                    break;
                case KEY__REL:
                    if (value != 0)
                        write(fds[EV_REL], event.eventChain, eventsSize);
                    break;
                case REL__KEY: // TODO do we have to take care of +- here?
                    write(fds[EV_KEY], event.eventChain, eventsSize);
                    break;
                case REL__REL:
                    event.eventChain[0].value = value;
                    write(fds[EV_REL], event.eventChain, eventsSize);
                    break;
                case ABS__ABS:
                    event.eventChain[0].value = value;
                    write(fds[fdnum], event.eventChain, eventsSize);
                    break;
                default:
                    break;
            }
            break;
        case MacroStart:
            switch (srcdst) {
                case KEY__:
                    if (extra.macroParts->parts[value] != nullptr)
                        extra.macroParts->parts[value]->proceed();
                    break;
                case OTHER:
                    extra.next->proceed();
                    break;
                default:
                    break;
            }
            break;


    }

}

void OutEvent::proceed() {
    if (outType == MacroPart)
        write(fds[fdnum], event.eventChain, eventsSize);
    else if (outType == Wait)
        usleep((__useconds_t) customValue); //TODO: make it non-blocking!

    if (extra.next != nullptr)
        extra.next->proceed();
}

OutEvent *OutEvent::setInputBits(QBitArray *inputBits[]) {
    switch (outType) {
        Wait:
            if (extra.ptr != nullptr)
                ((OutEvent *) extra.ptr)->setInputBits(inputBits);
            break;
        Macro:
            if (extra.ptr != nullptr)
                ((OutEvent *) extra.ptr)->setInputBits(inputBits);
        default:
            inputBits[EV_CNT]->setBit(eventtype);
            if (valueType == JoystickAxis && eventtype == EV_ABS)
                inputBits[EV_ABSJ]->setBit(eventcode);
            else
                inputBits[eventtype]->setBit(eventcode);
    }
    return this;
}


__u16 OutEvent::getSourceType() const {
    switch (outType) {
        case OutType::MacroStart:
            return extra.macroParts->parts[1]->getSourceType();
        case OutType::Wait:
            if (extra.next != nullptr)
                return extra.next->getSourceType();
        case OutType::Simple:
        case OutType::MacroPart:
            return (srcdst & SrcDst::INMASK) / 0x100;
        default:
            return EV_KEY; // TODO good idea?
    }
}

OutEvent::OutEvent(const OutEvent &other) {

}




