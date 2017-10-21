/*
* Created by arek on 10.10.17.
*/

#pragma once


#include "outsimple.h"

class OutDebounce : public OutSimple{
public:
    OutDebounce(QStringList l, __u16 sourceType);

    OutType type() const override { return OutType::Debounce;};

    void send(const __s32 &value, const timeval &time) override;

protected:
    int delay;
    int lastValue;
    timeval lastTime;
    long timeDiff(const timeval &newTime);
};



