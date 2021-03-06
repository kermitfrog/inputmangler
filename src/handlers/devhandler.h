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

#include "abstractinputhandler.h"
#include "output.h"
#include <QFile>

/*!
 * @brief DevHandler reads input from one input device and transforms it 
 * according to the rules set in the configuration file. (Saved in outputs)
 */
class DevHandler : public AbstractInputHandler
{
	// does this handle a mouse or a keyboard?
	// for devices that do both, 2 instances are created

public:
    DevHandler(AbstractInputHandler::idevs device);
    virtual ~DevHandler();
	static QList<AbstractInputHandler*> parseXml(pugi::xml_node &xml);
	virtual int getType() {return 1;};

protected:
	QString filename;
	int fd;        // file descriptor
	DType devtype; // keyboard or mouse
	input_absinfo * absmap[ABS_CNT]; // min/max values, etc. of absolute axes
	double absfac[ABS_CNT]; //!< multiplicator for absolute values
	int maxVal, minVal;     //!< min/max values of absolute axes in virtual output device
	
	void run();
    void sendAbsoluteValue(__u16 code, __s32 value);
};

