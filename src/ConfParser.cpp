/*  
    Created by arek on 2016-12-04.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>

*/

#include "ConfParser.h"

using namespace pugi;

ConfParser::ConfParser(QList<AbstractInputHandler *> *_handlers, QMap<QString, TransformationStructure> *_wsets) {
    handlers = _handlers;
    wsets = _wsets;
    readConf();
//    _handlers = handlers;
//    _wsets = wsets;
}

bool ConfParser::readConf() {
    //parse config
    QString confPath;
    if (QCoreApplication::arguments().count() < 2)
        confPath = QDir::homePath() + "/.config/inputMangler/config.xml";
    else
        confPath = QCoreApplication::arguments().at(1);
    xml_document confFile;
    xml_parse_result result = confFile.load_file(confPath.toUtf8().constData());
    if (result.status != status_ok)
    {
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
    setUpKeymaps(mapfileK, mapfileC, mapfileA);

    xml_node handlersConf = conf.child("handlers");
    for (xml_node handler = handlersConf.first_child(); handler; handler = handler.next_sibling()) {
        if (AbstractInputHandler::parseMap.contains(handler.name()))
        {
            qDebug() << "found <" << QString(handler.name());
            // all handler-specific configuration is read by the registered parsing functions
            QList<AbstractInputHandler*> inputHandler = AbstractInputHandler::parseMap[handler.name()](handler);
            handlers->append(inputHandler);
            if (inputHandler.size() > 0 && inputHandler.at(0)->id() != "")
                handlersById.insert(inputHandler.at(0)->id(), inputHandler.at(0));
        }
    }

    /// check if there is a reason to continue at all...
    if (handlers->count() == 0)
    {
        qDebug() << "no input Handlers loaded!";
        exit(1);
    }

    foreach(AbstractInputHandler *a, (*handlers))
    {
        if (!a->hasWindowSpecificSettings())
            continue;
        // make current outputs the default
        // if nessecary, a new TransformationStructure is created on demand by QMap,
        QVector<OutEvent> o = a->getOutputs();
        wsets->operator[](a->id()).def = o;
        ids.append(a->id());
    }
    ids.removeDuplicates();

    /// Now everything is prepared to read the Window-specific settings
    QMap<QString,QVector<OutEvent>> defOutputs;
    QMap<QString, bool> usedIds;
    foreach(QString id, ids) {
            defOutputs[id] = wsets->operator[](id).def;
            usedIds[id] = false;
        }

    parseWindowSettings(conf.child("windowConf"), defOutputs, usedIds);
    return true;
}

void ConfParser::parseWindowSettings(xml_node group, QMap<QString,QVector<OutEvent>> defaultOutputs, QMap<QString, bool> used) {

    for (xml_node entry = group.first_child(); entry; entry= entry.next_sibling()) {
        if (QString(entry.name()) == "window") {
            readWindowSettings(entry, defaultOutputs, used);
        } else if (QString(entry.name()) == "group") {
            QString s;
            QMap<QString, bool> usedIds = used;
            QMap<QString,QVector<OutEvent>> outputs = defaultOutputs;
            foreach (QString id, ids) {
                s = entry.attribute(id.toUtf8().data()).value();
                if (!s.isEmpty()) {
                    outputs[id] = parseOutputsShort(s, defaultOutputs[id]);
                    usedIds[id] = true;
                }
            }
            for (xml_node longDescription  = entry.child("long"); longDescription; longDescription = longDescription.next_sibling("long")) {
                QString id = longDescription.attribute("id").value();
                if (!longDescription.empty())
                {
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
void ConfParser::readWindowSettings(xml_node window, QMap<QString, QVector<OutEvent>> defaultOutputs, QMap<QString,bool> used) {
    QString windowClass = window.attribute("class").value();
    QString s;
    QMap<QString, WindowSettings*> wsettings; // <id, WindowSettings>
    QMap<QString, QMap<QString, unsigned int>> *inputsForIds;
    inputsForIds = static_cast<QMap<QString, QMap<QString, unsigned int>>*>(AbstractInputHandler::sd.infoCache["inputsForIds"]);

    if (inputsForIds->count() == 0)
        return;

    // create WindowSettings
    foreach(QString id, ids)
        wsettings.insert(id, new WindowSettings());

    foreach (QString id, ids) {
        s = window.attribute(id.toUtf8().data()).value();
        if (!s.isEmpty()) {
            wsettings[id]->def = parseOutputsShort(s, defaultOutputs[id]);
            used[id] = true;
        }
    }


    for (xml_node longDescription  = window.child("long"); longDescription; longDescription = longDescription.next_sibling("long")) {
        QString id = longDescription.attribute("id").value();
        if (!longDescription.empty())
        {
            WindowSettings *w = wsettings[id];
            w->def = parseOutputsLong(longDescription, handlersById[id], defaultOutputs[id]);
            used[id] = true;
        }
    }

    /// parse <title> nodes
    for (xml_node title  = window.child("title"); title; title = title.next_sibling("title")) {
        QString regex = title.attribute("regex").value();
        for (xml_attribute_iterator attr = title.attributes().begin(); attr != title.attributes().end(); ++attr ) {
            QString name = attr->name();
            if (name == "regex")
                continue;
            else if (ids.contains(name)) {
                wsettings[name]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
                wsettings[name]->events.append(parseOutputsShort(attr->value(), defaultOutputs[attr->value()]));
                used[name] = true;
            } else
                qDebug() << "Warning: unexpected attribute " + name + " in element " << windowClass << ", in title " + regex;
        }
        for (xml_node longDescription  = title.child("long"); longDescription; longDescription = longDescription.next_sibling("long")) {
            QString id = longDescription.attribute("id").value();
            if (!longDescription.empty())
            {
                wsettings[id]->titles.append(new QRegularExpression(QString("^") + regex + "$"));
                wsettings[id]->events.append(parseOutputsLong(longDescription, handlersById[id], defaultOutputs[id]));
                used[id] = true;
            }
        }
    }
    foreach(QString id, ids) {
        if (used[id] == true) {
            wsets->operator[](id).addWindowSettings(windowClass, wsettings[id]);
            if (wsettings[id]->def.size() == 0)
                wsettings[id]->def = defaultOutputs[id];
        }
        else
            delete wsettings.take(id);
    }
}

/*!
 * @brief Parse an output definition for window specific settings.
 * @param s string containing the output definitions.
 * @param defaults the parent's output definitions.
 * @return the parsed vector of OutEvents.
 */
QVector<OutEvent> ConfParser::parseOutputsShort(const QString str, QVector<OutEvent> &defaults) {
    QVector<OutEvent> vec;
    QStringList l = str.split(",");
    foreach(QString s, l) {
        if (s == "~") //inherit
            vec.append(defaults[vec.size() + 1]);
        else
            vec.append(OutEvent(s));
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
QVector<OutEvent> ConfParser::parseOutputsLong(xml_node node, const AbstractInputHandler * handler,
                                               QVector<OutEvent> def) {
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
        foreach (QString s, lines)
        {
            // we want to allow dual use of '=', e.g. <long> ==c, r== </long>
            s = s.trimmed();
            int pos = s.indexOf('=', 1);
            QString left = s.left(pos);
            QString right = s.mid(pos + 1);
            if (left.isEmpty() || right.isEmpty())
                continue;
            int index = handler->getInputIndex(left);

            if (index != -1)
                def[index] = OutEvent(right);
        }

    return def;


}


