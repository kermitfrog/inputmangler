/*
* Created by arek on 29.05.18.
*/

#pragma once

#include "../shared/keydefs.h"

class OutEvent;


extern QHash<QChar, OutEvent*> charmap; //!<  Used to map a character to a key sequence, e.g. A -> Shift+a in NetHandler
extern QHash<QString, OutEvent*> specialmap; //!< Used to map multicharacter sequences to a key sequence, e.g. ^R -> Ctrl+RIGHT
extern int max_sequence_length; //!<  length of the longest multicharacter input sequence in charmap.
extern QList<QChar> sequence_starting_chars; //!< list of all the characters, used to start a multicharacter sequence.

void setUpCharmap(QString charmap_path);
