/*
* Created by arek on 10.10.17.
*/

#pragma once


#include "outevent.h"

class OutAccel: public OutEvent {
public:
    OutAccel(QStringList l, __u16 sourceType);
    __u16 getSourceType() const override {
        return EV_REL; // TODO
    }
    OutType type() const override { return OutType::Accelerate;}

    void send(const __s32 &value, const timeval &time) override;

    void setInputBits(QBitArray **inputBits) override;;
protected:
    long int timeDiff(const timeval &newTime);

    int minKeyPresses;
    int maxDelay;
    float accelRate;
    float max;
    float currentRate;
    float overhead = 0.0;
    int triggered;

    __s32 normValue;  // 1 or -1
    timeval lastTime;

};



