/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2016-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>

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

#include "ConfParser.h"
#include "keydefs_charmap.h"

using namespace pugi;

/*!
 * @param _handlers pointer to list where configured handlers are to be stored
 * @param _wsets pointer to list where window specific configuration is to be stored
 */
ConfParser::ConfParser(QList<AbstractInputHandler *> *_handlers, QMap<QString, TransformationStructure> *_wsets) {
    handlers = _handlers;
    wsets = _wsets;

    evbits.resize(EV_CNT);
    keybits.resize(KEY_CNT);
    ledbits.resize(LED_CNT);
    relbits.resize(REL_CNT);
    absbitsT.resize(ABS_CNT);
    absbitsJ.resize(ABS_CNT);
    mscbits.resize(MSC_CNT);
    synbits.resize(SYN_CNT);

    inputBits[EV_CNT] = &evbits;
    inputBits[EV_KEY] = &keybits;
    inputBits[EV_LED] = &ledbits;
    inputBits[EV_REL] = &relbits;
    inputBits[EV_ABS] = &absbitsT;

    // Absolute Axes for a Joystick may have the same codes as those for Tablets, so we must seperate them
    // from Tablet events. Otherwise we won't know to which uinput device they belong later.
    inputBits[EV_ABSJ] = &absbitsJ;
    inputBits[EV_MSC] = &mscbits;
    inputBits[EV_SYN] = &synbits;

    specialSeq = QRegExp("(,|~[^,\\s])");

    readConf();
}

ConfParser::~ConfParser() {
}

/*!
 * actually reads the config
 * @return false if pugixml can't parse the file
 */
bool ConfParser::readConf() {
    //parse config
    QString confPath;
    if (QCoreApplication::arguments().count() < 2)
        confPath = QDir::homePath() + "/.config/inputMangler/config.xml";
    else
        confPath = QCoreApplication::arguments().at(1);
    xml_document confFile;
    xml_parse_result result = confFile.load_file(confPath.toUtf8().constData());
    if (result.status != status_ok) {
        qDebug() << result.description();
        return false;
    }

    xml_node conf = confFile.child("inputmanglerConf");

    // set up key definitions as well as char mappings and multi-char commands (nethandler)
    // this must be done *before* anything else is parsed.
    QString mapfileK, mapfileC, mapfileA;
    xml_node mapfiles = conf.child("mapfiles");
    mapfileK = mapfiles.attribute("keymap").value();
    mapfileC = mapfiles.attribute("charmap").value();
    mapfileA = mapfiles.attribute("axismap").value();
    qDebug() << "using keymap = " << mapfileK << ", charmap = " << mapfileC << ", axismap = " << mapfileA;
    setUpKeymaps(mapfileK, mapfileA);
    setUpCharmap(mapfileC);

    // set up all handlers
    xml_node handlersConf = conf.child("handlers");
    for (xml_node handler = handlersConf.first_child(); handler; handler = handler.next_sibling()) {
        if (AbstractInputHandler::parseMap.contains(handler.name())) {
            qDebug() << "found <" << QString(handler.name());
            // all handler-specific configuration is read by the registered parsing functions
            QList<AbstractInputHandler *> inputHandler = AbstractInputHandler::parseMap[handler.name()](handler);
            handlers->append(inputHandler);
            if (inputHandler.size() > 0 && inputHandler.at(0)->id() != "")
                handlersById.insert(inputHandler.at(0)->id(), inputHandler.at(0));
        }
    }

    /// check if there is a reason to continue at all...
    if (handlers->count() == 0) {
        qDebug() << "no input Handlers loaded!";
        std::exit(EXIT_FAILURE);
    }

    /// set up TransformationStructures
            foreach(AbstractInputHandler *a, (*handlers)) {
            a->setInputCapabilities(inputBits);
            if (!a->hasWindowSpecificSettings())
                continue;
            // make current outputs the default
            // if nessecary, a new TransformationStructure is created on demand by QMap,
            QVector<OutEvent *> o = a->getOutputs();
            wsets->operator[](a->id()).def = o;
            ids.append(a->id());
        }
    ids.removeDuplicates();

    /// Now everything is prepared to read the Window-specific settings
    QMap<QString, QVector<OutEvent *>> defOutputs;
    QMap<QString, bool> usedIds;
            foreach(QString id, ids) {
            defOutputs[id] = wsets->operator[](id).def;
            usedIds[id] = false;
        }

    parseWindowSettings(conf.child("windowConf"), defOutputs, usedIds);
    return true;
}

/*!
 * Parses one group of mappings
 *
 * <group> contains <window> or <group> subelements, for which this function is called recursively
 * mappings can be defined as attribute ([ID]="definitions") or <long> element with KEY="Output"
 * If a single key in <long> definition is omitted a defaultOutput will be used instead
 *
 * @param group xmlnode type "group" or "window"
 * @param defaultOutputs outputs of the element above in the hierachy (root defined in <signal> enties)
 * @param used
 */
void ConfParser::parseWindowSettings(xml_node group, QMap<QString, QVector<OutEvent *>> defaultOutputs,
                                     QMap<QString, bool> used) {

    for (xml_node entry = group.first_child(); entry; entry = entry.next_sibling()) {
        if (QString(entry.name()) == "window") {
            readWindowSettings(entry, defaultOutputs, used);
        } else if (QString(entry.name()) == "group") {
            QString s;
            QMap<QString, bool> usedIds = used;
            QMap<QString, QVector<OutEvent *>> outputs = defaultOutputs;
                    for (QString &id : ids) {
                    s = entry.attribute(id.toUtf8().data()).value();
                    if (!s.isEmpty()) {
                        outputs[id] = parseOutputsShort(s, defaultOutputs[id]);
                        usedIds[id] = true;
                    }
                }
            for (xml_node longDescription = entry.child(
                    "long"); longDescription; longDescription = longDescription.next_sibling("long")) {
                QString id = longDescription.attribute("id").value();
                if (!longDescription.empty()) {
                    outputs[id] = parseOutputsLong(longDescription, handlersById[id], defaultOutputs[id]);
                    usedIds[id] = true;
                }
            }
            parseWindowSettings(entry, outputs, usedIds);
        } else
            qDebug() << entry.name();
    }
}

/*!
 * @brief read window specific settings (a <window> element)
 * @var window <window> element as xml_node object
 * @var defaultOutputs output configuration of the parent
 */
void ConfParser::readWindowSettings(xml_node window, QMap<QString, QVector<OutEvent *>> defaultOutputs,
                                    QMap<QString, bool> used) {
    QString windowClass = window.attribute("class").value();
    QString s;
    QMap<QString, WindowSettings *> wsettings; // <id, WindowSettings>
    QMap<QString, QMap<QString, unsigned int>> *inputsForIds;
    inputsForIds = static_cast<QMap<QString, QMap<QString, unsigned int>> *>(AbstractInputHandler::sd.infoCache["inputsForIds"]);

    if (inputsForIds->count() == 0)
        return;

    // create WindowSettings
            for(const QString &id : ids)wsettings.insert(id, new WindowSettings());

            for (const QString &id : ids) {
            s = window.attribute(id.toUtf8().data()).value();
            if (!s.isEmpty()) {
                wsettings[id]->def = parseOutputsShort(s, defaultOutputs[id]);
                used[id] = true;
            }
        }


    for (xml_node longDescription = window.child(
            "long"); longDescription; longDescription = longDescription.next_sibling("long")) {
        QString id = longDescription.attribute("id").value();
        if (!longDescription.empty() && handlersById[id] != nullptr) {
            WindowSettings *w = wsettings[id];
            w->def = parseOutputsLong(longDescription, handlersById[id], defaultOutputs[id]);
            used[id] = true;
        }
    }

    /// parse <title> nodes
    for (xml_node title = window.child("title"); title; title = title.next_sibling("title")) {
        QString regex = title.attribute("regex").value();
        for (xml_attribute_iterator attr = title.attributes().begin(); attr != title.attributes().end(); ++attr) {
            QString name = attr->name();
            if (name == "regex")
                continue;
            else if (ids.contains(name)) {
                wsettings[name]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
                wsettings[name]->events.append(parseOutputsShort(attr->value(),
                                                                 defaultOutputs[attr->name()])); // attr->name() was attr->value() before. WTF?? is name wrong?
                used[name] = true;
            } else
                qDebug() << "Warning: unexpected attribute " + name + " in element " << windowClass
                         << ", in title " + regex;
        }
        for (xml_node longDescription = title.child(
                "long"); longDescription; longDescription = longDescription.next_sibling("long")) {
            QString id = longDescription.attribute("id").value();
			if (ids.contains(id) && !longDescription.empty()) {
                wsettings[id]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
                wsettings[id]->events.append(parseOutputsLong(longDescription, handlersById[id], defaultOutputs[id]));
                used[id] = true;
            }
        }
    }
            for(const QString &id : ids) {
            if (used[id]) {
                wsets->operator[](id).addWindowSettings(windowClass, wsettings[id]);
                if (wsettings[id]->def.empty())
                    wsettings[id]->def = defaultOutputs[id];
            } else
                delete wsettings.take(id);
        }
}

/*!
 * @brief Parse an output definition for window specific settings.
 * @param str string containing the output definitions.
 * @param defaults the parent's output definitions.
 * @return the parsed vector of OutEvents.
 */
QVector<OutEvent *> ConfParser::parseOutputsShort(const QString &str, QVector<OutEvent *> &defaults) {
    QVector<OutEvent *> vec;
    QStringList l = parseSplit(str);
    QString s;
    for (int i = 0; i < l.count(); ++i) {
        s = l.at(i);
        if (s == "~") //inherit
            vec.append(defaults[i]);
        else {
            OutEvent *outEvent = OutEvent::createOutEvent(s, defaults[i]->getSourceType());
            if (outEvent != nullptr) {
                vec.append(outEvent);
                outEvent->setInputBits(inputBits);
            } else {
                qDebug() << "Error in configuration File: can't create OutEvent for \"" << s << "\" in " << str;
                std::exit(EXIT_FAILURE);
            }
        }
    }
    return vec;
}

/*!
 * @brief Parse output definition for TransformationStructure.
 * Returns def when nothing is found.
 * @param node xml_node object of a <long> node
 * @param def Default value. Returned when no output definition is found. Result of
 * <long> description is based on this.
 */
QVector<OutEvent *> ConfParser::parseOutputsLong(xml_node node, const AbstractInputHandler *handler,
                                                 QVector<OutEvent *> def) {
    QString text = node.child_value();
    text = text.trimmed();
    if (text.isEmpty()) {
        qDebug() << "warning: empty <long> node!";
        return def;
    }
    // valid seperators are '\n' and ','
    QRegExp splitter;
    if (text.contains('~'))
        splitter = QRegExp("(\n)");
    else
        splitter = QRegExp("(\n|,)");

    QStringList lines = text.split(splitter, QString::SkipEmptyParts);
    for (QString s: lines) {
        // we want to allow dual use of '=', e.g. <long> ==c, r== </long>
        s = s.trimmed();
        int pos = s.indexOf('=', 1);
        QString left = s.left(pos);
        QString right = s.mid(pos + 1);
        if (left.isEmpty() || right.isEmpty())
            continue;
        int index = handler->getInputIndex(left);

        if (index != -1) {
            OutEvent *outEvent = OutEvent::createOutEvent(right, handler->getInputType(index));
            if (outEvent != nullptr) {
                def[index] = outEvent;
                def[index]->setInputBits(inputBits);
            }
        }
    }

    return def;


}

/**
 * Split a configuration string as needed for parsing (considering ',', '~[^,]*(' )
 * @param str value of [ID] attribute
 * @return a list of trimmed strings for further parsing by OutEvent::createOutEvent()
 */
QStringList ConfParser::parseSplit(const QString &str) const {
    QStringList list;
    QString part;
    QChar c;

    int pos, lastPos = 0;

    /* 4 cases: "~," "~..~)" "," "~$" but could contain whitespace chars
     * so the rules are:
     * lastPos is position after last seperating ','
     * 
     */
    do {
        pos = str.indexOf(noWS, lastPos);
        if (pos < 0)
            break;
        if (str.at(pos) == '~') {
            pos = str.indexOf(noWS, pos + 1);
            if (pos == -1) {
                list.append("~");
                break;
            }
            if (str.at(pos) == ',')
                list.append(str.mid(lastPos, pos - lastPos).trimmed());
            else {
                pos = str.indexOf("~)", pos) + 2;
                list.append(str.mid(lastPos, pos - lastPos).trimmed());
                pos = str.indexOf(',', pos);
            }
        } else {
            pos = str.indexOf(',', pos);
            list.append(str.mid(lastPos, pos - lastPos).trimmed());
        }
        lastPos = pos + 1;
    } while (lastPos > 0);

    return list;
}



