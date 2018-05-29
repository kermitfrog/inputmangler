/*
* Created by arek on 27.05.18.
*/

#pragma once


#include <QtCore/QAbstractItemModel>
#include <pugixml.hpp>

using namespace pugi;

class IMConfModel : public QAbstractItemModel {
    Q_OBJECT

public:
    class Device {
    public:
        Device() {};
        void fromNode(xml_node &n);
        QString name;
        QString id;
        QString dev;
        QStringList inputs;
        xml_node node;
    };

    IMConfModel(QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

protected:
    xml_document confFile;
    QVector<Device> devices;
    xml_node handlers;
    xml_node windows;


};



