/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Arkadiusz Guzinski <kermit@ag.de1.cc>

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
class OutEvent;

const int NUM_MOD = 5;

extern QHash<QString, int> keymap;
extern QHash<char, OutEvent> charmap;
extern QHash<QString, OutEvent> specialmap;

extern QMap<int, QString> keymap_reverse;

void setUpKeymaps();

// Flags

const int MOD_NONE		= 0x0;
const int MOD_SHIFT		= 0x1;
const int MOD_ALT		= 0x2;
const int MOD_CONTROL	= 0x4;
const int MOD_META		= 0x8;
const int MOD_ALTGR		= 0x10;
const int MOD_REPEAT	= 0x100;
const int MOD_MACRO		= 0x1000;



