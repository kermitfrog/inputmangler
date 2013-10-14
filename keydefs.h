#ifndef KEYDEFS_H
#define KEYDEFS_H

#include <linux/input.h>
#include <QHash>
#include <QString>
class TEvent;

const int NUM_MOD = 5;

extern QHash<QString, int> keymap;
extern QHash<char, TEvent> charmap;
extern QHash<QString, TEvent> specialmap;

void setUpKeymap();
void setUpCMap();
void setUpSMap();

// Flags

const int MOD_NONE		= 0x0;
const int MOD_SHIFT		= 0x1;
const int MOD_ALT		= 0x2;
const int MOD_CONTROL	= 0x4;
const int MOD_META		= 0x8;
const int MOD_ALTGR		= 0x10;
const int MOD_REPEAT	= 0x100;
const int MOD_MACRO		= 0x1000;



#endif
