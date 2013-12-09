#include "keydefs.h"
#include "abstractinputhandler.h"

QHash<QString, int> keymap;
QHash<char, OutEvent> charmap;
QHash<QString, OutEvent> specialmap;

void setUpKeymaps()
{
	QMap<QString, int> in;
	QFile input_h_file("/usr/include/linux/input.h");
	if (!input_h_file.open(QIODevice::ReadOnly))
	{
		qDebug() << "could not load keysyms from " << input_h_file.fileName();
		return;
	}
	QTextStream input_h_stream(&input_h_file);
	QStringList input_h = input_h_stream.readAll().split("\n"); 
// 	input_h_file.close();
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
					in[tmp[1]] = code;
			}
		li++;
	}
	
	QString confPath = QDir::homePath() + "/.config/inputMangler/keymap";
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
				code = in[tmp[0]];
			else 
				code = tmp[0].toInt();
			keymap[tmp[1]] = code;
		}
		li++;
	}
	
	charmap['\n']        = OutEvent(KEY_ENTER);
	charmap['\r']        = OutEvent(KEY_ENTER);
	charmap['\b']     = OutEvent(KEY_BACKSPACE);
	charmap['\t']   = OutEvent(KEY_TAB);
	charmap[' ']     = OutEvent(KEY_SPACE);
	
	confPath = QDir::homePath() + "/.config/inputMangler/charmap";
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
		if (tmp.size() == 1)
		{
			charmap[tmp[0][0].toLatin1()] = OutEvent(keymap[tmp[0]]);
#ifdef DEBUGME
			qDebug() << tmp[0] << "=" << tmp[0] << " --> " << charmap[tmp[0][0].toLatin1()].print() ;
#endif
		}
		else if(tmp[0].size() == 1)
		{
			charmap[tmp[0][0].toLatin1()] = OutEvent(tmp[1]);
#ifdef DEBUGME
			qDebug() << tmp[0] << "=" << tmp[1] << " --> " << charmap[tmp[0][0].toLatin1()].print() ;
#endif
		}
		else 
		{
			specialmap[tmp[0]] = OutEvent(tmp[1]);
#ifdef DEBUGME
			qDebug() << tmp[0] << "=" << tmp[0] << " --> " << specialmap[tmp[0]].print() ;
#endif
		}
		li++;
	}
	
	
/*	
	charmap['\x0e4']     = TEvent(KEY_A, false, true);
	charmap['\x0f6']     = TEvent(KEY_S, false, true);
	charmap['\x0fc']     = TEvent(KEY_F, false, true);
	charmap['\x0df']     = TEvent(KEY_SEMICOLON, false, true);

	charmap['\x0c4']     = TEvent(KEY_A, true, true);
	charmap['\x0d6']     = TEvent(KEY_S, true, true);
	charmap['\x0dc']     = TEvent(KEY_F, true, true);
*/

}
