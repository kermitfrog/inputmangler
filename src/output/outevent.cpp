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

#include "outevent.h"
#include <fcntl.h>
#include <unistd.h>
#include <QList>
#include "keydefs.h"
#include "outsimple.h"
#include "outmacrostart.h"
#include "outwait.h"
#include "outaccel.h"
#include "outauto.h"
#include "outdebounce.h"
#include "outmacropart.h"
#include "macropartbase.h"

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
        parts = s.left(s.indexOf("~)")).split(',', QString::SkipEmptyParts);
        for (int i = 0; i < parts.size(); ++i)
            parts[i] = parts[i].trimmed();
        if (otype == "Seq")
            return new OutMacroStart(parts, sourceType);
        if (otype == "+")
            return new OutAccel(parts, sourceType);
        if (otype.startsWith("A"))
            return new OutAuto(parts, sourceType);
        if (otype.startsWith("D"))
            return new OutDebounce(parts, sourceType);
    }

    return nullptr;
}

OutEvent *OutEvent::createOutEvent(InputEvent &s, __u16 sourceType) {
    return new OutSimple(s, sourceType);
};

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


    for (i = s.length() - 1; i >= 0; --i) {
        macro.append(QString(s.at(i)) + " 0");
    }

    return macro;
}

void OutEvent::send(const __s32 &value, const timeval &time) {
    qDebug() << "send called for " << type() << " with value " << value;
}

void OutEvent::sendRaw(input_event &event, DType dtype) {
    write(fds[dtype], &event, sizeof(input_event));
}



