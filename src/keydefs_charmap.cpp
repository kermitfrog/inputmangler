/*
* Created by arek on 29.05.18.
*/

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <output/outevent.h>
#include "keydefs_charmap.h"

QHash<QChar, OutEvent*> charmap;
QHash<QString, OutEvent*> specialmap;
int max_sequence_length;
QList<QChar> sequence_starting_chars;

void setUpCharmap(QString charmap_path) {
    charmap.clear();
    specialmap.clear();

    QString confPath;
    QStringList tmp;
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

    QList<QString>::iterator li = charmap_text.begin();
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
            charmap[tmp[0][0]] = OutEvent::createOutEvent(keymap[tmp[0]], EV_REL); // TODO is EV_REL a good choice?
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
                charmap[left[0]] = OutEvent::createOutEvent(right, EV_REL); // TODO is EV_REL a good choice?
#ifdef DEBUGME
                qDebug() << tmp[0] << "=" << "s" << " --> " << charmap[tmp[0][0].toLatin1()].print() ;
#endif
            }
            else // case 3. "^D PAGEDOWN"
            {
                specialmap[left] = OutEvent::createOutEvent(right, EV_REL); // TODO is EV_REL a good choice?
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
