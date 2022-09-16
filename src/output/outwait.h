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


#include "macropartbase.h"

/**
 * represents the delay part of a macro.
 * "~s [delay in milliseconds]"
 */
class OutWait : public MacroPartBase {

public:
    OutWait(QStringList &macroParts, __u16 sourceType);
    __s32 time; ///<! in microseconds

    OutType type() const override { return OutType::Wait;};

    __u16 getSourceType() const override;

    QString toString() const override;

    void proceed() override;
};


