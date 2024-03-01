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

#pragma once


#include "../../shared/inputevent.h"
#include "../../shared/definitions.h"
#include <linux/input.h> //__u16...
#include <QString>
#include <QVector>
#include <linux/uinput.h>
#include <QBitArray>
#include <QDebug>
#include <unistd.h>

#define MacroValDevider "~|"


class OutSimple;

/*!
 * @brief An output event, e.g: 
 * "S" for press shift
 * "g" for press g
 * "d+C" for press Ctrl-D
 * "~Seq(BTN_LEFT 1, ~s50, MOUSE_X 150, ~s50, BTN_LEFT 0 ~)" for Drag&Drop 150 pixels to the right
 * more Complex Types may be supported in the future (maybe via plugins)
 */
class OutEvent {
public:
    /*!
     * @brief OutType is the type of the OutEvent.
     */
    enum OutType {
        Simple,        //!< A single event. e.g. "key a" - the value is determined by source
        MacroStart,    //!< Complex event sequence.
        MacroPart,  //!< Part of Complex event sequence.
        Wait,        //!< Wait a while
        Repeat,        //!< Autofire / Repeat
        Accelerate, //!< For mouse wheel acceleration (additional presses, if triggered fast)
        Debounce,   //!< Hack for broken buttons
        AbsRel,     //!< Experiment - for Joystick to Mouse in Ambermoon
        Custom,        //!< Handle via plugin. Not yet implemented.
        Invalid     //!< Returned on error in createOutEvent()
    };
    /*!
     * @brief Type of Src and Destination device
     */
    enum SrcDst { // TODO values should depend on definitions in linux_input.h
        KEY__ABS = 0,
        REL__ABS = 0b1011,
        ABS__KEY = 0,
        ABS__REL = 0b1110,
        KEY__KEY = 0b0101,
        KEY__REL = 0b0110,
        REL__KEY = 0b1001,
        REL__REL = 0b1010,
        ABS__ABS = 0b1111,
        KEY__ = 0b0100,
        REL__ = 0b1000,
        ABS__ = 0b1100,
        OTHER = 0b10000,
        INMASK = 0b1100
    };
    union Events {
        input_event **eventChains; //!< KEY__KEY only
        input_event *eventChain;
        int value;
    };

    friend class WindowSettings;

    friend class TransformationStructure;

public:
    static OutEvent *createOutEvent(QString s, __u16 sourceType);

    static OutEvent *createOutEvent(InputEvent &s, __u16 sourceType);

    virtual OutType type() const { return OutType::Invalid; };

    virtual QString toString() const { return QString::number(type()); };

    QString print() const { return toString(); };
    Events event;

    size_t eventsSize;

    SrcDst srcdst;

    virtual void send(const __s32 &value, const timeval &time);

    void send() {
        timeval t;
        __s32 v = 1;
        send(v, t);
    }; //!< only used by nethandler // TODO t = now? ; is this alright?

    // protected?

    static void sendRaw(input_event &event, DType dtype = Auto);

    static void sendRawSafe(__u16 type, __u16 code, __s32 value, DType dtype = Auto); // TODO
//    static void sync(int device);

    virtual bool isValid() { return true; }; // TODO actually implement

    static void generalSetup(QBitArray *inputBits[]);

    static void closeVirtualDevices();

    static int fds[5]; //!< file descriptors for /dev/virtual_*: None, Keyboard, Mouse, Tablet, Joystick
#ifdef DEBUGME
    QString initString;
#endif

    virtual void setInputBits(QBitArray **inputBits);

    virtual __u16 getSourceType() const = 0;

    static void cleanUp();

protected:
    void setSrcDst(__u16 src, __u16 dst) { srcdst = (SrcDst) ((src * 4) + dst); };

    void invalidate(QString message = "") {
        qDebug() << message;
        eventsSize = 0;
    };

    static void setSync(input_event &e) {
        e.type = EV_SYN;
        e.code = SYN_REPORT;
        e.value = 0;
    };

    static QList<QStringList> comboToMacro(QStringList list);

    static uinput_user_dev *makeUinputUserDev(const char *name, __u16 vId = 0, __u16 pId = 0);

    __u8 fdnum; //<! which fds[] to write to
    void interpretNegSource(__u16 &sourceType);

    __s8 valueMod = 1;

    void registerEvent(); //<! register for later delete - should be called in constructor, unless the event is deleted by another event
    static QList<OutEvent *> registeredEvents;
};



