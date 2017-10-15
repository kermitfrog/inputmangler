/*
* Created by arek on 07.10.17.
*/

#pragma once

#include "outevent.h"
#include "macropartbase.h"

class OutEvent;

class MacroPartBase : public OutEvent {
public:
    virtual void proceed() = 0;
    static MacroPartBase* parseMacro(QStringList &macroParts, __u16 sourceType);
protected:
    MacroPartBase * next = nullptr;
};



