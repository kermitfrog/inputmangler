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

#include <linux/input.h>
#include <QHash>
#include <QMap>
#include <QString>
class InputEvent;

// Pconst int NUM_MOD = 5; //!<  maximum number of mmodifiers in an OutEvent[K] @deprecated ?

extern QHash<QString, InputEvent> keymap; //!< Used to map a key in the configuration file to a keycode.

extern QMap<int, QString> keymap_reverse; //!< Reverse mapped keymap (for debugging output), where int = code+(10000*type)+(1000*vtype)

void setUpKeymaps(QString keymap_path, QString axis_path);
QString getStringForCode(__u16 code, __u16 type, __u8 fdnum);

// Flags - currently not used, and maybe never will be.
// const int MOD_NONE		= 0x0;
// const int MOD_SHIFT		= 0x1;
// const int MOD_ALT		= 0x2;
// const int MOD_CONTROL	= 0x4;
// const int MOD_META		= 0x8;
// const int MOD_ALTGR		= 0x10;
// const int MOD_REPEAT		= 0x100;
// const int MOD_MACRO		= 0x1000;



