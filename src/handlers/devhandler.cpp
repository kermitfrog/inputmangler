/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "devhandler.h"
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

using namespace pugi;
const int buffer_size = 4;

/*!
 * @brief This thread will read from one input device and transform input
 * events according to previosly set rules.
 */
void DevHandler::run() {
    // TODO toLatin1 works... always? why not UTF-8?
    fd = open(filename.toLatin1(), O_RDONLY);
    if (fd == -1) {
        qDebug() << "?could (not?) open " << filename;
        return;
    }

    // grab the device, otherwise there will be double events
    ioctl(fd, EVIOCGRAB, 1);

    struct pollfd p;
    p.fd = fd;
    p.events = POLLIN;
    p.revents = POLLIN;

    int ret; // return value of poll
    ssize_t n;   // number of events to read / act upon
    bool matches; // does it match an input code that we want to act upon?

    input_event buf[buffer_size];
    while (true) {
        // wait until there is data or 1.5 seconds have passed to look at sd.terminating
        ret = poll(&p, 1, 1500);
        if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            qDebug() << "Device " << _id << "was removed or other error occured!"
                     << "\n shutting down " << _id;
            break;
        }

        //break the loop if we want to stop
        if (sd.terminating) {
            qDebug() << "terminating";
            break;
        }
        //if we did not wake up due to timeout...
        if (ret) {
            n = read(fd, buf, buffer_size * sizeof(input_event));
            for (int i = 0; i < n / sizeof(input_event); i++) {
                matches = false;
                // Key/Button and movements(relative and absolute) only
                // We do not handle misc and sync events
                // new: ABS passthrough (and some experimental modification)
                if (buf[i].type >= EV_KEY && buf[i].type <= EV_ABS)
                    for (int j = 0; j < outputs.size(); j++) {
                        //qDebug() << j << " of " << outputs.size() << " in " << id();
                        if (inputs[j] == buf[i]) {
                            matches = true;
#ifdef DEBUGME
                            qDebug() << "Output : " << outputs.at(j).initString;
#endif
                            outputs[j]->send(buf[i].value, buf[i].time);
                            break;
                        }
                    }
                // Pass through - absolute movements need translation
                /*if (buf[i].type == EV_ABS)
                    sendAbsoluteValue(buf[i]);
                else */
                if (!matches)
                    OutEvent::sendRaw(buf[i], devtype);
// 				qDebug("type: %d, code: %d, value: %d", buf[i].type, buf[i].code, buf[i].value );
            }
            //qDebug() << id << ":" << n;
        }
    }

    // unlock & close
    ioctl(fd, EVIOCGRAB, 0);
    close(fd);
}

/*!
 * @brief translate and send an absolute value. Translation is needed because
 * source and destination devices have different minimum and maximum values for axes.
 * @param code Axis
 * @param value Value
 */
void DevHandler::sendAbsoluteValue(input_event &ev) {
// 	qDebug() << "sendAbsoluteValue: orig = "  << value;
/*    ev.value -= absmap[ev.code]->minimum;
    ev.value *= absfac[ev.code];
    ev.value += minVal;*/
// 	if (devtype == Tablet)
// 		qDebug() << "sendAbsoluteValue(" << type << code << orig
// 		  << ") min1: " << absmap[code]->minimum << "fac: " << absfac[code] 
// 		  << "minVal: " << minVal << "==> " << value;
    OutEvent::sendRaw(ev, devtype);
}

/*!
 * @brief Construct a DevHandler thread for the device described in device.
 * @param device Description of a device.
 */
DevHandler::DevHandler(idevs device) {
    this->sd = sd;
    _id = device.id;
    _hasWindowSpecificSettings = _id != "";
    filename = QString("/dev/input/") + device.event;
    devtype = device.type;
    QStringList dnames = {"Auto", "Keyboard", "Mouse", "Tablet", "Joystick", "Tablet and Joystick"};
    qDebug() << "Opening " << device.event << " as " << dnames.at(devtype);
}

/*!
 * @brief Parses a <device> part of the configuration and constructs DevHandler objects.
 * @param xml pugi::xml_node object at current position of a <device> element.
 * @return List containing all DevHandlers. This can contain multiple objects because some
 * devices have multiple event handlers. In this case a thread is created for every event handlers.
 */
QList<AbstractInputHandler *> DevHandler::parseXml(pugi::xml_node &xml) {
    QMap<QString, QMap<QString, unsigned int>> *inputsForIds;
    if (!sd.infoCache.contains("inputsForIds")) {
        inputsForIds = new QMap<QString, QMap<QString, unsigned int>>();
        sd.infoCache["inputsForIds"] = inputsForIds;
    } else
        inputsForIds = static_cast<QMap<QString, QMap<QString, unsigned int>> *>(sd.infoCache["inputsForIds"]);
    unsigned int counter = 0;

    QList<AbstractInputHandler *> handlers;
    // get a list of available devices from /proc/bus/input/devices
    QList<idevs> availableDevices = parseInputDevices();
    /// Devices
    /*
     * create an idevs structure for the configured device
     * vendor and product are set to match the devices in /proc/bus/...
     * id is the config-id
     */
    idevs d;
    d.readAttributes(xml);

    // create a devhandler for every device that matches vendor and product
    // of the configured device,
    while (availableDevices.count(d)) {
        int idx = availableDevices.indexOf(d);
        // copy information obtained from /proc/bus/input/devices to complete
        // the data in the idevs object used to construct the DevHandler
        d.event = availableDevices.at(idx).event;
        d.type = availableDevices.at(idx).type;
        DevHandler *devhandler = new DevHandler(d);
        availableDevices.removeAt(idx);
        handlers.append(devhandler);
    }

    /*
     * read the <signal> entries.
     * [key] will be the input event, that will be transformed
     * [default] will be the current output device, this is
     * transformed to. If no [default] is set, the current output will
     * be the same as the input.
     * For DevHandlers with window specific settings, the current output
     * becomes the default output when the TransformationStructure is
     * constructed, otherwise it won't ever change anyway...
     */

    for (xml_node signal = xml.child("signal"); signal; signal = signal.next_sibling("signal")) {

        QString key = signal.attribute("key").value();
        QString def = signal.attribute("default").value();
                foreach(AbstractInputHandler *a, handlers) {
                DevHandler *devhandler = static_cast<DevHandler *>(a);
                InputEvent ie = keymap[key];
                if (def == "")
                    devhandler->addInput(ie);
                else {
                    OutEvent *outEvent = OutEvent::createOutEvent(def, ie.type); // TODO is there something to do for Joystick support?
                    if (outEvent != nullptr)
                        devhandler->addInput(ie, outEvent);
                    else {
                        qDebug() << "Error in configuration File: can't create OutEvent for Input default \"" << def << "\"";
                        std::exit(EXIT_FAILURE);
                    }
                }
            }
        inputsForIds->operator[](d.id)[key] = counter++;
    }

    return handlers;
}

/**
 * Determines which flags uinput needs to be able to generate events for this device. This is neccessary for passing
 * through unhandled events.
 *
 * @param inputBits where to set the flags; see ConfParser
 * @return information about absolute axes (min/max values) of this device
 */
input_absinfo**  DevHandler::setInputCapabilities(QBitArray **inputBits) {
    qDebug() << "DevHandler::setInputCapabilities() called for " << filename;
    fd = open(filename.toLatin1(), O_NONBLOCK);
    if (fd == -1) {
        qDebug() << "?could (not?) open " << filename;
        return nullptr;
    }
    input_absinfo ** absinfo;
    __u8 types[(EV_CNT+7)/8]; // round up...
    __u8 buff[(KEY_CNT+7)/8]; // round up...
    ioctl(fd, EVIOCGBIT(0, (EV_CNT+7)/8), types);

    if (isBitSet(types, EV_KEY)) {
        inputBits[EV_CNT]->setBit(EV_KEY);
        ioctl(fd, EVIOCGBIT(EV_KEY, (KEY_CNT + 7) / 8), buff);
        for (int i = 0; i < KEY_CNT; i++)
            if (isBitSet(buff, i))
                inputBits[EV_KEY]->setBit(i);
    }

    if (isBitSet(types, EV_LED)) {
        inputBits[EV_CNT]->setBit(EV_LED);
        ioctl(fd, EVIOCGBIT(EV_LED, (LED_CNT + 7) / 8), buff);
        for (int i = 0; i < LED_CNT; i++)
            if (isBitSet(buff, i))
                inputBits[EV_LED]->setBit(i);
    }

    if (isBitSet(types, EV_REL)) {
        inputBits[EV_CNT]->setBit(EV_REL);
        ioctl(fd, EVIOCGBIT(EV_REL, (REL_CNT + 7) / 8), buff);
        for (int i = 0; i < REL_CNT; i++)
            if (isBitSet(buff, i))
                inputBits[EV_REL]->setBit(i);
    }

    //get abs information. minVal and maxVal corrospond to what is set in inputdummy
/*    if (devtype == Tablet || devtype == Joystick || devtype == TabletOrJoystick) {
        if (devtype == Tablet) {
            minVal = 0;
            maxVal = 32767;
        } else {
            minVal = -255;
            maxVal = 255;
        }

        for (int i = 0; i < ABS_CNT; i++) {
            absmap[i] = new input_absinfo;

            if (absmap[i]->maximum == absmap[i]->minimum)
                absfac[i] = 0.0;
            else
                absfac[i] = ((double) maxVal - (double) minVal) /
                            ((double) absmap[i]->maximum - (double) absmap[i]->minimum);
        }
    }*/
    if (isBitSet(types, EV_ABS)) {
        inputBits[EV_CNT]->setBit(EV_ABS);
        //absinfo = new input_absinfo[ABS_CNT];
        ioctl(fd, EVIOCGBIT(EV_ABS, (ABS_CNT + 7) / 8), buff);
        for (int i = 0; i < ABS_CNT; i++)
            if (isBitSet(buff, i)) {
                inputBits[EV_ABS]->setBit(i);
                if (ioctl(fd, EVIOCGABS(i), absmap[i]) == -1)
                    qDebug() << "Unable to get absinfo for axis " << i;

            }
    }

    if (isBitSet(types, EV_MSC)) {
        inputBits[EV_CNT]->setBit(EV_MSC);
        ioctl(fd, EVIOCGBIT(EV_MSC, (MSC_CNT + 7) / 8), buff);
        for (int i = 0; i < MSC_CNT; i++)
            if (isBitSet(buff, i))
                inputBits[EV_MSC]->setBit(i);
    }

    if (isBitSet(types, EV_SYN)) {
        inputBits[EV_CNT]->setBit(EV_SYN);
        ioctl(fd, EVIOCGBIT(EV_SYN, (SYN_CNT + 7) / 8), buff);
        for (int i = 0; i < SYN_CNT; i++)
            if (isBitSet(buff, i))
                inputBits[EV_SYN]->setBit(i);
    }

    close(fd);
    return absinfo;
}

DevHandler::~DevHandler() {

}

