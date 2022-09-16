/*
* Created by arek on 21.05.19.
*/

#pragma once


#include "outevent.h"

class OutAbsRel : public OutEvent{
public:
    OutAbsRel(QStringList l, __u16 sourceType);
    ~OutAbsRel();

    __s32 currentPos;
    float factor;
//    __s32 center;
//    int min, max;

    __u16 getSourceType() const override;

    bool isValid() override;

    void send(const __s32 &value, const timeval &time) override;
};



