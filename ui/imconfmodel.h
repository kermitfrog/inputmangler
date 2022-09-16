/*
* Created by arek on 27.05.18.
*/

#pragma once


#include <QtCore/QAbstractItemModel>
#include <pugixml.hpp>
#include <QtCore/QLinkedList>
#include <QtGui/QFont>

using namespace pugi;

class IMConfModel : public QAbstractItemModel {
Q_OBJECT

public:
    const int OutputRole = Qt::UserRole + 0x0100;

    class Device {
    public:
        Device() {};

        void fromXmlNode(xml_node &n);

        QString name;
        QString id;
        QString dev;
        QStringList inputs;
        xml_node node;
    };

    class Node {
    public:
        enum NodeType {
            Root, Group, Window, Title
        };

        Node(xml_node reference);

        ~Node() { for (Node *n : children) delete (n); }

        xml_node xml;
        Node *parent;
        NodeType type;
        QVector<Node *> children;

        int getRow(Node *node);

        QString name();

        void parse();
    };

    IMConfModel(QObject *parent);

    ~IMConfModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

public slots:
    void test(QModelIndex index);

    const QVector<Device> &getDevices() const;

protected:
    const int NUM_COLS = 1;
    xml_document confFile;
    QVector<Device> devices;
    xml_node handlers;
    Node *windows;

    Node *getNode(QModelIndex &index);

    QFont groupFont;


};



