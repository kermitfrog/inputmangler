/*
* Created by arek on 06.10.17.
*/

#pragma once


#include "outevent.h"
//#include "macropartbase.h"

class MacroPartBase;

class OutMacroStart : public OutEvent{
public:
    OutMacroStart(QStringList l, __u16 sourceType);
    virtual const OutType type() { return OutType::MacroStart;}
    MacroPartBase * macroParts[3];

    void send(const __s32 &value, const timeval &time) override;;

    __u16 getSourceType() const override;

    void setInputBits(QBitArray **inputBits) override;

};



