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

#include "keydefs.h"
#include "abstractinputhandler.h"

QHash<QString, InputEvent> keymap;
QHash<char, OutEvent> charmap;
QHash<QString, OutEvent> specialmap;
QMap<int, QString> keymap_reverse;
unsigned int max_sequence_length;
QList<QChar> sequence_starting_chars;

/*!
 * @brief this global function reads /usr/include/linux/input.h, keymap and charmap and populates the globally used maps(see keydefs.h).
 */
void setUpKeymaps(QString keymap_path, QString charmap_path, QString axis_path)
{
	// clear them all, just in case we're rereading
	keymap.clear();
	charmap.clear();
	specialmap.clear();
	keymap_reverse.clear();
	
	/*
	 * read /usr/include/linux/input.h
	 * find all lines like
	 * #define KEY_MINUS       12
	 * and make a mapping of "KEY_MINUS" to 12
	 */
	QMap<QString, int> inputMap; // Mappings from input.h
	QFile input_h_file("/usr/include/linux/input.h");
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
	while (li != axis_text.end())
	{
		tmp = (*li).split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (tmp.size() >= 3)
		{
			int code, type;
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
			}
			
			keymap[tmp[2]] = InputEvent(code, type, vtype);
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
	
	/*
	 * read ~/.config/inputMangler/charmap and populate charmap or specialmap.
	 * there are three possibilities per line:
	 * 1. "$" : Character '$' is mapped to the output event associated with itself in keymap. (charmap)
	 * 2. "^ @+S" : Character '^' is mapped to OutEvent("@+^"). (charmap)
	 * 3. "^D PAGEDOWN" : The character sequence "^D" is mapped to OutEvent("PAGEDOWN"). (specialmap)
	 * On the left side it is possible to map any Unicode character by using a 4-digit hex code like \u09ab.
	 */
	
	max_sequence_length = 1;
	
	if (charmap_path.isEmpty())
		confPath = QDir::homePath() + "/.config/inputMangler/charmap";
	else
		confPath = charmap_path;
	QFile charmap_file(confPath);
	if (!charmap_file.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not load keysyms from " << charmap_file.fileName();
		return;
	}
	QTextStream charmap_stream(&charmap_file);
	QStringList charmap_text = charmap_stream.readAll().split("\n"); 
	
	li = charmap_text.begin();
	while (li != charmap_text.end())
	{
		tmp = (*li).split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (tmp.size() == 0)
		{
			li++;
			continue;
		}
		QString left = tmp[0];
		// interpret character codes in 4-byte hex notation introduced by \u
		while (left.contains("\\u"))
		{
			bool ok;
			int pos = left.indexOf("\\u");
			QString s = left.mid(pos + 2, 4);
			if (s.length() != 4)
			{
				qDebug() << "Error in charmap at interpreting " << left;
				break;
			}
			left.replace(pos, 6, s.toUShort(&ok, 16));
			if (!ok)
			{
				qDebug() << "Error in charmap at interpreting " << left;
				break;
			}
		}
		// case 1. "$"
		if (tmp.size() == 1)
		{
			charmap[tmp[0][0].toLatin1()] = OutEvent(keymap[tmp[0]]);
#ifdef DEBUGME
			qDebug() << tmp[0] << "=" << tmp[0] << " --> " << charmap[tmp[0][0].toLatin1()].print();
#endif
		}
		else 
		{
			QString right = tmp[1];
			// case 2. "^ @+S"
			if(left.size() == 1)
			{
				charmap[left[0].toLatin1()] = OutEvent(right);
	#ifdef DEBUGME
				qDebug() << tmp[0] << "=" << "s" << " --> " << charmap[tmp[0][0].toLatin1()].print() ;
	#endif
			}
			else // case 3. "^D PAGEDOWN"
			{
				specialmap[left] = OutEvent(right);
				if (right.length() > max_sequence_length)
					max_sequence_length = left.length();
				if (!sequence_starting_chars.contains(left.at(0)))
					sequence_starting_chars.append(left.at(0));
	#ifdef DEBUGME
				qDebug() << tmp[0] << "=" << tmp[0] << " --> " << specialmap[tmp[0]].print() ;
	#endif
			}
		}
		li++;
	}
	charmap_file.close();
	qSort(sequence_starting_chars);
}
