/*
* Created by arek on 06.10.17.
*/

#include "outmacrostart.h"
#include "macropartbase.h"

OutMacroStart::OutMacroStart(QStringList l, __u16 sourceType) {
    qDebug() << "Parsing OutMacroStart: sourceType=" << sourceType << ", l=" << l;
    QStringList press, repeat, release;
    for (int i = 0; i < l.size(); ++i)
        l[i] = l[i].trimmed();

    if (sourceType == EV_KEY)
        srcdst = KEY__;
    else
        srcdst = OTHER;

    switch (l.count(MacroValDevider)) {
        case 0:
            press = l;
            break;
        case 1:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            release = l;
            break;
        case 2:
            while (l.at(0) != MacroValDevider)
                press.append(l.takeFirst());
            l.removeFirst();
            while (l.at(0) != MacroValDevider)
                repeat.append(l.takeFirst());
            l.removeFirst();
            release = l;
            break;
        default:
            invalidate("too many \"" + QString(MacroValDevider) + "\" in " + l.join(", "));
    }
    
    qDebug() << "Parsing OutMacroStart: l=" << l << ", press=" << press << ", release=" << release << ", repeat=" << repeat;
    qDebug() << "Parsing OutMacroStart: sizes: press=" << press.size() << ", release=" << release.size() << ", repeat=" << repeat.size();

    
    
    if (release.size() == 0 && repeat.size() == 0) {
        if (press.size() > 0)
            macroParts[1] = MacroPartBase::parseMacro(press, sourceType);
    } else {
        if (press.size() > 0)
            macroParts[1] = MacroPartBase::parseMacro(press, sourceType);
        else
            macroParts[1] = nullptr;

        if (release.size() > 0)
            macroParts[0] = MacroPartBase::parseMacro(release, sourceType);
        else
            macroParts[0] = nullptr;

        if (repeat.size() > 0)
            macroParts[2] = MacroPartBase::parseMacro(repeat, sourceType);
        else
            macroParts[2] = nullptr;
    }
}

void OutMacroStart::send(const __s32 &value, const timeval &time) {
    qDebug() << "Start with value=" << value << ", srcdst=" << srcdst;
    switch (srcdst) {
        case KEY__:
            if (macroParts[value] != nullptr)
                macroParts[value]->proceed();
            break;
        case OTHER:
            macroParts[1]->proceed();
            break;
        default:
            break;
    }


}

__u16 OutMacroStart::getSourceType() const {
    return macroParts[1]->getSourceType();
}
