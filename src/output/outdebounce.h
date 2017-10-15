/*
* Created by arek on 10.10.17.
*/

#pragma once


#include "outevent.h"

class OutDebounce : public OutEvent{
public:
    OutDebounce(QStringList l, __u16 sourceType);
    __u16 getSourceType() const override {
        return EV_KEY; // TODO
    }
    OutType type() const override { return OutType::Debounce;};

};



