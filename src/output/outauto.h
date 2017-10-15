/*
* Created by arek on 10.10.17.
*/

#pragma once
#include "outevent.h"

class OutAuto: public OutEvent {
public:
    OutAuto(QStringList l, __u16 sourceType);
    __u16 getSourceType() const override {
        return EV_KEY; // TODO
    }

    OutType type() const override { return OutType::Repeat;};

    void send(const __s32 &value, const timeval &time) override;

};



