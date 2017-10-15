/*
*  Created by arek on 06.10.17.
*/

#pragma once

#include "outevent.h"

class OutEvent;

class OutSimple : public OutEvent {

public:
    OutSimple(InputEvent& e, __u16 sourceType);
    ~OutSimple();
    virtual OutEvent::OutType type() const { return OutType::Simple;};
    virtual void send(const __s32 &value, const timeval &time);

    void setInputBits(QBitArray **inputBits) override;

    __u16 getSourceType() const override;

protected:



};


