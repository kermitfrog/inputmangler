#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include "imconfmodel.h"
#include "../shared/keydefs.h"
#include <QDebug>

/*
* Created by arek on 27.05.18.
*/
using namespace pugi;

IMConfModel::IMConfModel(QObject *parent) : QAbstractItemModel(parent) {

    // #START copy from ConfParser::readConf
    QString confPath;
    if (QCoreApplication::arguments().count() < 2)
        confPath = QDir::homePath() + "/.config/inputMangler/config.xml";
    else
        confPath = QCoreApplication::arguments().at(1);
    xml_parse_result result = confFile.load_file(confPath.toUtf8().constData());
    if (result.status != status_ok) {
        qDebug() << result.description();
        return;
    }

    xml_node conf = confFile.child("inputmanglerConf");

    // set up key definitions as well as char mappings and multi-char commands (nethandler)
    // this must be done *before* anything else is parsed.
    QString mapfileK, mapfileC, mapfileA;
    xml_node mapfiles = conf.child("mapfiles");
    mapfileK = mapfiles.attribute("keymap").value();
    mapfileC = mapfiles.attribute("charmap").value();
    mapfileA = mapfiles.attribute("axismap").value();
    setUpKeymaps(mapfileK, mapfileC, mapfileA);
    // #END copy from ConfParser::readConf

    // parse handlers
    handlers = conf.child("handlers");
    for (xml_node device = handlers.first_child(); device; device = device.next_sibling()) {
        devices.append(Device());
        devices.last().fromNode(device);
    }

    windows = conf.child("windowConf");
}

void IMConfModel::Device::fromNode(xml_node &n) {
    node = n;
    id = n.attribute("id").value();
    name = n.attribute("name").value();
    if (name.isEmpty())
        name = id;
    // TODO dev =
    for (xml_node signal = node.first_child(); signal; signal = node.next_sibling()) {
        inputs.append(signal.attribute("key").value());
    }

}

QModelIndex IMConfModel::index(int row, int column, const QModelIndex &parent) const {
    return QModelIndex();
}

QModelIndex IMConfModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int IMConfModel::rowCount(const QModelIndex &parent) const {
    return 0;
}

int IMConfModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant IMConfModel::data(const QModelIndex &index, int role) const {
    return QVariant();
}
