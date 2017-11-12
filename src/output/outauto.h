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
 * Variant of OutSimple that autofires (press + release) on a repeat event from it's source
 * only works if both, the source and the destination event are of type EV_KEY.
 * Otherwise it behaves just like OutSimple.
 */
class OutAuto: public OutSimple {
public:
    OutAuto(QStringList l, __u16 sourceType);
    __u16 getSourceType() const override {
        return EV_KEY; // TODO
    }

    OutType type() const override { return OutType::Repeat;};

    void send(const __s32 &value, const timeval &time) override;

    QString toString() const override;
};



