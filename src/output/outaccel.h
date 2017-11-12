/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2016-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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


#include "outevent.h"

/**
 * An accelerated event. If this makes sense for anything except a scroll wheel, please let me know.
 * Supports only Relative axis events as source.
 * Attempt to accelerate mouse scrolling. algorithm needs to be improved.
 */
class OutAccel: public OutEvent {
public:
    OutAccel(QStringList l, __u16 sourceType);
    __u16 getSourceType() const override {
        return EV_REL; // TODO
    }
    OutType type() const override { return OutType::Accelerate;}

    void send(const __s32 &value, const timeval &time) override;

    QString toString() const override;

    void setInputBits(QBitArray **inputBits) override;;
protected:
    long int timeDiff(const timeval &newTime);

    int minKeyPresses; //!< has to be triggered at least so many times with less than maxDelay in between before we accelerate
    int maxDelay;      //!< maximum delay before we reset the counter
    float accelRate;   //!< by how much to accelerate (0.2 means 1 -> 1.2 -> 1.4 -> 1.6
    float max;         //!< maximum acceleration
    float currentRate;
    float overhead = 0.0;
    int triggered;

    __s32 normValue;  // 1 or -1
    timeval lastTime;

    const int MinKeyPressDefault = 2;
    const int MaxDelayDefault = 400;
    const float AccelRateDefault = 2.25;
    const float MaxDefault = 15.0;
};



