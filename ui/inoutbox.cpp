/*
* Created by arek on 08.06.18.
*/

#include "inoutbox.h"

InOutBox::InOutBox(QWidget *parent) : QGroupBox(parent) {
    QVBoxLayout *layout = new QVBoxLayout();
    this->setLayout(layout);
    grid = new QGridLayout();
    layout->addLayout(grid);
}

void InOutBox::setDevice(IMConfModel::Device &dev) {
    device = dev;
    int i = 0;
    QString label;

    setTitle(dev.name);
    for (xml_node node = dev.node.first_child(); node; node = node.next_sibling()) {
        if (!node.attribute("label").empty())
            label = QString::fromStdString(node.attribute("label").value());
        else
            label = QString::fromStdString(node.attribute("key").value());

        InOut io( label, QString::fromStdString(node.attribute("default").value()));
        inouts.append(io);
        QLabel *in = new QLabel(this);
        QLineEdit *out = new QLineEdit(this);
        in->setText(io.key);
        out->setText(io.defaultOut);
        grid->addWidget(in, i, 0);
        grid->addWidget(out, i, 1);
        i++;
    }
}

void InOutBox::setData(xml_node *xml) {

}
