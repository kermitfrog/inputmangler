/*
* Created by arek on 06.10.17.
*/

#pragma once


#include "outevent.h"
#include "macropartbase.h"

class OutMacroPart: public MacroPartBase{
public:

    OutMacroPart(QStringList &macroParts, __u16 sourceType);
    virtual const OutType type() { return OutType::MacroPart;}
    __u16 getSourceType() const override;

    void setInputBits(QBitArray **inputBits) override;

    void proceed() override;
};



