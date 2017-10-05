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

#pragma once

#include "inputevent.h"
#include "definitions.h"
#include <linux/input.h> //__u16...
#include <QString>
#include <QVector>
#include <linux/uinput.h>
#include <QBitArray>
#include <QDebug>

#define MacroValDevider "~|"

/*!
 * @brief An output event, e.g: 
 * "S" for press shift
 * "g" for press g
 * "d+C" for press Ctrl-D
 * "~Seq(BTN_LEFT 1, ~s50, MOUSE_X 150, ~s50, BTN_LEFT 0 ~)" for Drag&Drop 150 pixels to the right
 * more Complex Types will be supported in the future (maybe via plugins)
 */
class OutEvent 
{
	/*!
	 * @brief OutType is the type of the OutEvent.
	 */
	enum OutType {
		Simple,	    //!< A single event. e.g. "key a" - the value is determined by source
		MacroStart,	//!< Complex event sequence.
		MacroPart,  //!< Part of Complex event sequence.
		Wait,	    //!< Wait a while
		Repeat,	    //!< Autofire / Repeat
		Accelerate, //!< For mouse wheel acceleration (additional presses, if triggered fast)
        Debounce,   //!< Hack for broken buttons
		Custom	    //!< Handle via plugin. Not yet implemented.
	};
	/*!
	 * @brief Type of Src and Destination device
	 */
	enum SrcDst { // TODO values should depend on definitions in linux_input.h
		KEY__ABS = 0,
		REL__ABS = 0,
		ABS__KEY = 0,
		ABS__REL = 0,
		KEY__KEY = 0x0100,
		KEY__REL = 0x0101,
		REL__KEY = 0x1000,
		REL__REL = 0x1001,
		ABS__ABS = 0x1110,
		KEY__    = 0x0100,
		REL__    = 0x1000,
		ABS__    = 0x1100,
		OTHER    = 0x1000,
		INMASK   = 0x1100
	};
    struct AccelSettings
    {
        int minKeyPresses;
        int maxDelay;
        float accelRate;
        float max;
        float currentRate;
		float overhead = 0.0;
    };
	struct MacroParts
	{
		OutEvent *parts[3];
	};
    union CustomVar
    {
        void * ptr = nullptr;
		OutEvent * next;
        int integer;
		MacroParts * macroParts;
    };
	union Events
	{
		input_event ** eventChains; //!< KEY__KEY only
		input_event * eventChain;
        int value;
	};

	friend class WindowSettings;
	friend class TransformationStructure;

public:
	OutEvent() {};
	OutEvent(InputEvent& e, __u16 sourceType);
	OutEvent(QString s, __u16 sourceType);
	OutEvent(const OutEvent &other);
	OutEvent(OutEvent &other);
	~OutEvent();
// 	OutEvent(__s32 code, bool shift, bool alt = false, bool ctrl = false);
	QString toString() const;
	QString print() const{return toString();};
	QVector<__u16> modifiers;
    // input_event * event[];
	Events event;

	size_t eventsSize;


	__u16 eventtype;
	__u16 eventcode;
	__u16 code() const {return eventcode;};
    timeval time; // last triggered at that time
	OutType outType;
	SrcDst srcdst;
	ValueType valueType;
	int customValue = 0;  		//!< on Macro: custom value, on Wait: time in microseconds
	void send(const __s32 &value, const timeval &time);
	void send() {timeval t; __s32 v = 1; send(v, t);}; //!< only used by nethandler // TODO t = now? ; is this alright?

	// protected?

	static void sendRaw(__u16 type, __u16 code, __s32 value, DType dtype = Auto);
	static void sendRawSafe(__u16 type, __u16 code, __s32 value, DType dtype = Auto);
    static void sync(int device);

	bool isValid() { return true;}; // TODO actually implement

	static void generalSetup(QBitArray* inputBits[]);
	static void closeVirtualDevices();
	static int fds[5]; //!< file descriptors for /dev/virtual_*: None, Keyboard, Mouse, Tablet, Joystick
#ifdef DEBUGME
	QString initString;
#endif
	//OutEvent& operator=(OutEvent &other);
	OutEvent& operator=(const OutEvent &other);
    OutEvent* setInputBits(QBitArray* inputBits[]);

	__u16 getSourceType() const;
    
protected:
	OutEvent(QStringList list, __u16 sourceType); //!< for Macroparts
	void sendMacro();
	void sendSimple(int value);
	void sendCombo(int value);
	void fromInputEvent(InputEvent& e);
	//static void openVDevice(const char * path, int num);
	CustomVar extra;  //!< Next event in macro sequence or pointer to custom event or additional variable
	void proceed();
	void parseCombo(QStringList l);
	void parseMacro(QStringList l, __u16 sourceType);
	OutEvent(QStringList &macroParts, __u16 sourceType);
    int timeDiff(timeval &newTime);

    void parseAcceleration(QStringList params);

    void sendAccelerated(int value, timeval &newTime);

    void sendDebounced(int value, timeval &newTime);

    void invalidate(QString message = "") {qDebug() << message; };

	static void setSync(input_event &e) {e.type = EV_SYN; e.code = SYN_REPORT; e.value = 0;};
	static QStringList comboToMacro(QStringList list);

    static uinput_user_dev* makeUinputUserDev(char *name);

	__u8 fdnum;
};



