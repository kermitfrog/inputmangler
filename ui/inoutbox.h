/*
* Created by arek on 08.06.18.
*/

#pragma once


#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <pugixml.hpp>
#include <QtWidgets/QGridLayout>
#include "imconfmodel.h"

using namespace pugi;

class InOutBox : public QGroupBox {
    Q_OBJECT
    QVector<QLabel*> inputs;
    QVector<QLineEdit*> outputs;

    class InOut {
    public:
        InOut(){};
        InOut(const QString &key, const QString &defaultOut) : key(key)
            {if (defaultOut.isEmpty()) this->defaultOut = key; else this->defaultOut = defaultOut;};
        QString defaultOut;
        QString key;
    };

public:
    InOutBox(QWidget *parent);
    void setDevice(IMConfModel::Device &dev);

    void setData(xml_node *xml);

protected:
    IMConfModel::Device device;
    QVector<InOut> inouts;
    QGridLayout *grid;
};



