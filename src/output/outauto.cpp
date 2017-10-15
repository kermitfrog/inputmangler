/*
* Created by arek on 10.10.17.
*/

#include "outauto.h"

OutAuto::OutAuto(QStringList l, __u16 sourceType) {

}

void OutAuto::send(const __s32 &value, const timeval &time) {
    OutEvent::send(value, time);
}
