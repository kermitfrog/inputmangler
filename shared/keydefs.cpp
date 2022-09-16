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

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QDebug>
#include <QtCore/QDir>
#include "keydefs.h"
#include "definitions.h"
#include "inputevent.h"
//#include "handlers/abstractinputhandler.h"

QHash<QString, InputEvent> keymap;
QMap<int, QString> keymap_reverse;

/*!
 * @brief this global function reads /usr/include/linux/input.h, keymap and charmap and populates the globally used maps(see keydefs.h).
 */
void setUpKeymaps(QString keymap_path, QString axis_path)
{
	// clear them all, just in case we're rereading
	keymap.clear();
	keymap_reverse.clear();
	
	/*
	 * read /usr/include/linux/input.h
	 * find all lines like
	 * #define KEY_MINUS       12
	 * and make a mapping of "KEY_MINUS" to 12
	 */
	QMap<QString, int> inputMap; // Mappings from input.h

    QString input_h_file_name = "/usr/include/linux/input-event-codes.h";
    if (!QFile::exists(input_h_file_name)) // before the input.h - split
        input_h_file_name = "/usr/include/linux/input.h";

    QFile input_h_file(input_h_file_name);
    if (!input_h_file.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not load keysyms from " << input_h_file.fileName();
		return;
	}
	QTextStream input_h_stream(&input_h_file);
	QStringList input_h = input_h_stream.readAll().split("\n"); 
 	input_h_file.close();
	QStringList tmp;
	
	QList<QString>::iterator li = input_h.begin();
	while (li != input_h.end())
	{
		tmp = (*li).split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (tmp.size() >= 3)
			if (tmp[0] == "#define")
			{
				int code;
				bool ok;
				if (tmp[2].startsWith("0x"))
					code = tmp[2].toInt(&ok, 16);
				else 
					code = tmp[2].toInt(&ok, 10);
				if (ok)
					inputMap[tmp[1]] = code;
			}
		li++;
	}
	
	/*
	 * read ~/.config/inputMangler/keymap
	 * empty lines are ignored, otherwise ' ' is the delimiter
	 * KEY_3 { => map value of "KEY_3" in inputMap to "{"
	 */
	QString confPath;
	if (keymap_path.isEmpty())
		confPath = QDir::homePath() + "/.config/inputMangler/keymap";
	else
		confPath = keymap_path;
	QFile keymap_file(confPath);
	if (!keymap_file.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not load keysyms from " << keymap_file.fileName();
		return;
	}
	QTextStream keymap_stream(&keymap_file);
	QStringList keymap_text = keymap_stream.readAll().split("\n"); 
// 	input_h_file.close();
	
	li = keymap_text.begin();
	while (li != keymap_text.end())
	{
		tmp = (*li).split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (tmp.size() >= 2)
		{
			int code;
			if (QRegExp("\\D").exactMatch(tmp[0].left(1)) )
				code = inputMap[tmp[0]];
			else 
				code = tmp[0].toInt();
			keymap[tmp[1]] = code;
			keymap_reverse[code] = tmp[1];
		}
		li++;
	}
	keymap_file.close();
	
	/*
	 * read ~/.config/inputMangler/axismap
	 * empty lines are ignored, otherwise ' ' is the delimiter
	 * REL_WHEEL + WHEEL_UP { => map positive values of "REL_WHEEL" in inputMap to "WHEEL_UP"
	 */
	if (axis_path.isEmpty())
		confPath = QDir::homePath() + "/.config/inputMangler/axismap";
	else
		confPath = axis_path;
	QFile axis_file(confPath);
	if (!axis_file.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not load keysyms from " << axis_file.fileName();
		return;
	}
	QTextStream axis_stream(&axis_file);
	QStringList axis_text = axis_stream.readAll().split("\n"); 
	
	li = axis_text.begin();
    int min, max;
	while (li != axis_text.end())
	{
        min = max = 0;
		tmp = (*li).split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (tmp.size() >= 3)
		{
			int type;
			__u16 code;
			ValueType vtype;
			if (QRegExp("\\D").exactMatch(tmp[0].left(1)) )
				code = inputMap[tmp[0]];
			else 
				code = tmp[0].toInt();
			if (tmp[0].startsWith("REL_"))
				type = EV_REL;
			else if (tmp[0].startsWith("ABS_"))
				type = EV_ABS;
			else
				continue;
			
			// see definitions.h for documentation of value types.
			switch (tmp[1][0].toLatin1()) {
				case '*':
					vtype = All;
					break;
				case '+':
					vtype = Positive;
					break;
				case '-':
					vtype = Negative;
					break;
				case '0':
					vtype = Zero;
					break;
				case 'T':
					vtype = TabletAxis;
                    if (tmp.length() >= 5) {
                        min = tmp[3].toInt();
						max = tmp[4].toInt();
                    } else {
						max = 0x7fff;
					}
					break;
				case 'J':
					vtype = JoystickAxis;
					if (tmp.length() >= 5) {
						min = tmp[3].toInt();
						max = tmp[4].toInt();
					} else {
						min = -0x7ffe;
						max = 0x7fff;
					}
					break;
				default:
					vtype = All;
			}
			
			keymap[tmp[2]] = InputEvent(code, type, vtype, min, max);
 			keymap_reverse[code+(10000*type)+(1000*vtype)] = tmp[2];
		}
		li++;
	}
	axis_file.close();

#ifdef DEBUGME
	foreach (QString t, keymap.keys())
	{
		qDebug() << t << " == " << keymap[t].print();
	}
#endif
	
}

QString getStringForCode(__u16 code, __u16 type, __u8 fdnum) {
	int key = code + (10000 * type);
	return keymap_reverse[key];
}

