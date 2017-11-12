/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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


#include "outsimple.h"

/**
 * Variant of OutSimple, that can be used to work around broken, jittery, keys by only triggering an event if a certain
 * amount of time has passed since the last time it was triggered.
 */
class OutDebounce : public OutSimple{
public:
    OutDebounce(QStringList l, __u16 sourceType);

    QString toString() const override;

    OutType type() const override { return OutType::Debounce;};

    void send(const __s32 &value, const timeval &time) override;

protected:
    int delay; //<! delay in microseconds
    int lastValue; //<! last triggered value
    timeval lastTime; //<! time of last call to send()
    long timeDiff(const timeval &newTime);
};



