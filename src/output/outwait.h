/*
* Created by arek on 07.10.17.
*/

#pragma once


#include "macropartbase.h"

class OutWait : public MacroPartBase {

public:
    OutWait(QStringList &macroParts, __u16 sourceType);
    __s32 time;

    OutType type() const override { return OutType::Wait;};

    __u16 getSourceType() const override;

    void proceed() override;
};



