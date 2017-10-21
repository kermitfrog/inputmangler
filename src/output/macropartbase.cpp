#include "macropartbase.h"
#include "outmacropart.h"
#include "outwait.h"
#include <QDebug>

/*
* Created by arek on 07.10.17.
*/
MacroPartBase *MacroPartBase::parseMacro(QStringList &macroParts, __u16 sourceType) {
    if (macroParts.isEmpty())
        return nullptr;

    QString s = macroParts.first();
    if (s.startsWith('~')) {
        if (s.length() < 2) {
            qDebug() << ("'~' without followup in Macro configuration " + macroParts.join(", "));
            return nullptr;
        } else if (s.at(1) == 's') {
            return new OutWait(macroParts, sourceType);
        } else {
            qDebug() << ("unknown directive \"" + s + "\" in Macro configuration " + macroParts.join(", "));
            return nullptr;
        }
    }

    return new OutMacroPart(macroParts, sourceType);
}

void MacroPartBase::setInputBits(QBitArray **inputBits) {
    if (next != nullptr)
        next->setInputBits(inputBits);
};


