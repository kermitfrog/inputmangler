/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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

#include "outevent.h"
#include <fcntl.h>
#include <unistd.h>
#include <QList>
#include "../../shared/keydefs.h"
#include "outsimple.h"
#include "outmacrostart.h"
#include "outwait.h"
#include "outaccel.h"
#include "outauto.h"
#include "outdebounce.h"
#include "outmacropart.h"
#include "macropartbase.h"
#include "outabsrel.h"

int OutEvent::fds[5];
QList<OutEvent*> OutEvent::registeredEvents;

/*!
 * @brief opens virtual devices for output
 * @param inputBits which flags to set on uinput devices; see ConfParser.h
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
    err |= ioctl(fd, UI_SET_EVBIT, EV_MSC) | ioctl(fd, UI_SET_MSCBIT, MSC_TIMESTAMP);
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
            qDebug() << strerror(errno);
            qDebug() << sizeof(*dev);
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
//                    dev->absmin[i] = ie.absmin;
//                    dev->absmax[i] = ie.absmax;
                    // as this does not work properly... set to SpaceMouse values for now
                    dev->absmin[i] = -350;
                    dev->absmax[i] = 350;
                    dev->absflat[i] = 5 ;
                    dev->absfuzz[i] = 0;
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

#ifdef DEBUGME
    qDebug() << "kbd: " << OutEvent::fds[1] << ", mouse: " << OutEvent::fds[2];
    qDebug() << "tablet: " << OutEvent::fds[3] << ", joystick: " << OutEvent::fds[4];
#endif
}

/**
 * @brief sets uinput flags neccessary for this Event; might have to be reimplemented in inherited class
 * @param inputBits stores which flags to set on uinput devices; see ConfParser.h
 */
void OutEvent::setInputBits(QBitArray **inputBits) {
    __u16 evType, code;
    if (eventsSize == 0)
        return;
    if (srcdst == KEY__KEY) {
        evType = event.eventChains[1][0].type;
        code = event.eventChains[1][0].code;
    } else {
        evType = event.eventChain[0].type;
        code = event.eventChain[0].code;
    }
    inputBits[EV_CNT]->setBit(evType);
    if (fdnum == 4 && evType == EV_ABS)
        inputBits[EV_ABSJ]->setBit(code);
    else
        inputBits[evType]->setBit(code);
}

/**
 * @brief creates virtual input device
 * @param name identifies device later (e.g. in /proc/bus/devices)
 * @return
 */
uinput_user_dev *OutEvent::makeUinputUserDev(const char *name) {
    uinput_user_dev *dev;
    dev = new uinput_user_dev;
    memset(dev, 0, sizeof(*dev));
    dev->id.bustype = BUS_USB;
    dev->id.vendor = dev->id.product = 0;
    dev->id.version = 1;
    snprintf(dev->name, UINPUT_MAX_NAME_SIZE, "%s", name);
    return dev;
}

/*!
 * @brief destroys virtual devices
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
 *      ~Rel      (Absolute to Relative)
 *
 * @param s configuration String
 * @param sourceType type of the event that triggers this OutEvent (as configured in <signal key= >)
 * @return created OutEvent
 */
OutEvent *OutEvent::createOutEvent(QString s, __u16 sourceType) {
    s = s.trimmed();
    if (s.isEmpty())
        return nullptr;
    QStringList parts;
    if (s[0] != '~') {
        parts = s.split('+');
        if (parts.count() == 1)
            return new OutSimple(keymap[parts[0]], sourceType);
        if (parts.count() == 2)
            return new OutMacroStart(comboToMacro(parts), sourceType);
    } else {
        int brackPos = s.indexOf('(');
        if (brackPos < 0) {
            qDebug() << "Parsing error: missing '(' in " << s;
            exit(EXIT_FAILURE);
        }
        QString otype = s.mid(1, brackPos - 1);
        s.remove(0, brackPos + 1);
        if (otype == "Seq") {
            QList<QStringList> mParts;
            QStringList list = s.left(s.indexOf("~)")).split(MacroValDevider, QString::SkipEmptyParts);
            while (!list.isEmpty())
                mParts.append(list.takeFirst().split(',', QString::SkipEmptyParts));
            return new OutMacroStart(mParts, sourceType);
        } else {
            parts = s.left(s.indexOf("~)")).split(',', QString::SkipEmptyParts);
            for (int i = 0; i < parts.size(); ++i)
                parts[i] = parts[i].trimmed();
            if (otype == "+")
                return new OutAccel(parts, sourceType);
            if (otype.startsWith("A"))
                return new OutAuto(parts, sourceType);
            if (otype.startsWith("D"))
                return new OutDebounce(parts, sourceType);
            if (otype.startsWith("R"))
                return new OutAbsRel(parts, sourceType);
        }
    }

    return nullptr;
}

/*!
 * wrapper to create a Simple OutEvent from an InputEvent
 * @param s Event to create from
 * @param sourceType type of the event that triggers this OutEvent (as configured in <signal key= >)
 * @return created OutEvent
 */
OutEvent *OutEvent::createOutEvent(InputEvent &s, __u16 sourceType) {
    return new OutSimple(s, sourceType);
};

/*!
 * @brief translates a split Combo configuration String to a Macro configuration String (
 * Example:
 * "a+S" == ["a","S"] -> ["S 1", "a 1", "~|", "a 2", "~|", "a 0", "S 0"]
 * @param list split Combo configuration String in format ["KEY", "ACS"], where each letter of "ACS" is one Modifier
 * @return split Macro configuration String
 */
QList<QStringList> OutEvent::comboToMacro(QStringList list) {
    QList<QStringList> macro;
    QStringList part;
    QString s;
    int i;

    // press modifiers
    s = list.at(1);
    for (i = 0; i < s.length(); ++i) {
        part.append(QString(s.at(i)) + " 1");
    }
    // press KEY
    part.append(list.at(0) + " 1"); // TODO what happens if list.at(0) is not EV_KEY?
    macro.append(part);
    part.clear();

    // repeat KEY only
    part.append(list.at(0) + " 2");
    macro.append(part);
    part.clear();

    // release KEY
    part.append(list.at(0) + " 0");

    // release modifiers
    for (i = s.length() - 1; i >= 0; --i) {
        part.append(QString(s.at(i)) + " 0");
    }
    macro.append(part);

    return macro;
}

/*!
 * @brief send an event; usualy called by handler; default implementation just writes params to STDERR
 * @param value value from input
 * @param time time the input event was generated by the kernel
 */
void OutEvent::send(const __s32 &value, const timeval &time) {
    qDebug() << "send called for " << type() << " with value " << value;
}

/*!
 * generates an event on a virtual device
 * this is used to pass nonhandled events through and does no checks
 * @param event raw input event
 * @param dtype which device to send it to (1: Keyboard, 2: Mouse, 3: Tablet, 4: Joystick)
 */
void OutEvent::sendRaw(input_event &event, DType dtype) {
    write(fds[dtype], &event, sizeof(input_event));
}

/**
 * sets valueMod to -1 if needed.
 * This is neccessary if the source is a negative axis
 * see OutSimple::init() for an example
 * @param sourceType
 */
void OutEvent::interpretNegSource(__u16 &sourceType) {
    if (sourceType > NegativeModifier) {
        sourceType -= NegativeModifier;
        valueMod = -1;
    }
}

/**
 * Send an input event to uinput through inputmangler. For use through DBUS.
 * @param type
 * @param code
 * @param value
 * @param dtype
 */
void OutEvent::sendRawSafe(__u16 type, __u16 code, __s32 value, DType dtype) {
    qDebug() << "void OutEvent::sendRawSafe(__u16 type, __u16 code, __s32 value, DType dtype)";
    input_event e;
    e.type = type;
    e.code = code;
    e.value = value;
    __u8 fd;
    switch (dtype)
    {
        case Keyboard:
        case Mouse:
        case Tablet:
        case Joystick:
            fd = dtype;
            break;
        case Auto:
            if (code >= BTN_MOUSE)
                if (fds[Mouse])
                    fd = Mouse;
                else
                if (fds[e.type])
                    fd = dtype;
            break;
        case TabletOrJoystick:
            if (fds[Tablet])
                fd = Tablet;
            else if (fds[Joystick])
                fd = Joystick;
            break;
        default:
            return;
    };
    if (fds[fd]) {
        sendRaw(e, (DType)fd);
        setSync(e);
        sendRaw(e, (DType)fd);
    }
}


void OutEvent::registerEvent() {
    registeredEvents.append(this);
}

void OutEvent::cleanUp() {
    qDebug() << registeredEvents.count() << " events will be deleted";
    while (!registeredEvents.empty())
        delete registeredEvents.takeFirst();
}






