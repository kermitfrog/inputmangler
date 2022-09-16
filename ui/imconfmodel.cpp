#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <imconfmodel.h>
#include <../shared/keydefs.h>
#include <QDebug>
#include <QSize>

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
    // mapfileC = mapfiles.attribute("charmap").value();
    mapfileA = mapfiles.attribute("axismap").value();
    setUpKeymaps(mapfileK, mapfileA);
    // #END copy from ConfParser::readConf

    // parse handlers
    handlers = conf.child("handlers");
    for (xml_node device = handlers.child("device"); device; device = device.next_sibling("device")) {
        devices.append(Device());
        devices.last().fromXmlNode(device);
    }

    windows = new Node(conf.child("windowConf"));
    windows->parse();
    windows->parent = nullptr;

    groupFont.setBold(true);

}

void IMConfModel::test(QModelIndex index) {
    qDebug() << "TEST: " << index << "  " << data(index, Qt::DisplayRole);
}

void IMConfModel::Node::parse() {
    QString name;
    for (xml_node child = xml.first_child(); child; child = child.next_sibling()) {
        name = QString().fromStdString(child.name());
        if (name == "group" || name == "window" || name == "title") {
            Node *node = new Node(child);
            node->parse();
            node->parent = this;
            children.append(node);
        }
    }
}

IMConfModel::~IMConfModel() {
    delete windows;
}

void IMConfModel::Device::fromXmlNode(xml_node &n) {
    node = n;
    id = n.attribute("id").value();
    name = n.attribute("name").value();
    if (name.isEmpty())
        name = id;
    // TODO dev =
    //for (xml_node signal = node.child("signal"); signal; signal = node.next_sibling("signal")) {
    for (xml_node signal = node.first_child(); signal; signal = signal.next_sibling()) {
        inputs.append(signal.attribute("key").value());
    }

}

QModelIndex IMConfModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Node *parentItem;

    if (!parent.isValid())
        parentItem = windows;
    else
        parentItem = static_cast<Node *>(parent.internalPointer());

    Node *childItem = parentItem->children[row];
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex IMConfModel::parent(const QModelIndex &child) const {
    if (!child.isValid())
        return QModelIndex();

    Node *node = reinterpret_cast<Node *>(child.internalPointer());
    node = node->parent;

    if (node == windows || node == nullptr)
        return QModelIndex();
    return createIndex(node->parent->getRow(node), 0, node);
}

int IMConfModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return reinterpret_cast<Node *>(parent.internalPointer())->children.size();
    return windows->children.size();
}

int IMConfModel::columnCount(const QModelIndex &parent) const {
    return NUM_COLS;
}

QVariant IMConfModel::data(const QModelIndex &index, int role) const {
    // TODO test and maybe use Q_DECLARE_METATYPE(), possibly use role too
    Node *node = reinterpret_cast<Node *>(index.internalPointer());
    if (role == Qt::DisplayRole)
        return node->name();
    if (role == Qt::UserRole)
        return QVariant::fromValue(index.internalPointer());
    if (role == Qt::FontRole && node->type == Node::Group)
        return groupFont;
    if (role >= OutputRole)
        return QVariant(node->xml);

    return QVariant();
}

IMConfModel::Node *IMConfModel::getNode(QModelIndex &index) {
    index.parent();
}

const QVector<IMConfModel::Device> &IMConfModel::getDevices() const {
    return devices;
}

int IMConfModel::Node::getRow(IMConfModel::Node *node) {
    return children.indexOf(node);
}

IMConfModel::Node::Node(xml_node reference) {
    xml = reference;
    QString eName = QString::fromStdString(xml.name());
    if (eName == "window")
        type = Window;
    else if (eName == "title")
        type = Title;
    else if (eName == "group") // Make Bold
        type = Group;
    else
        type = Root;

}

QString IMConfModel::Node::name() {
    switch (type) {
        case Window:
            return QString::fromStdString(xml.attribute("class").value());
        case Title:
            return "\"" + QString::fromStdString(xml.attribute("regex").value()) + "\"";
        case Group:
            return ">" + QString::fromStdString(xml.attribute("name").value()) + "<";
        default:
            return "root / invalid";
    }
}

#include "moc_imconfmodel.cpp"
