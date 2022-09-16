/*
* Created by arek on 27.05.18.
*/

#include "inputmanglerui.h"
#include "inoutbox.h"

InputmanglerUI::InputmanglerUI() {

    ui = new Ui_InputmanglerUI();
    ui->setupUi(this);
    model = new IMConfModel(this);
    ui->windowTreeView->setModel(model);
    ui->windowTreeView->expandToDepth(0);

    connect(ui->windowTreeView, SIGNAL(activated(QModelIndex)), model, SLOT(test(QModelIndex)));

    QVBoxLayout *layout = new QVBoxLayout();
    ui->scrollAreaWidgetContents->setLayout(layout);

    for (IMConfModel::Device dev : model->getDevices()) {
        InOutBox *inOutBox = new InOutBox(this);
        layout->addWidget(inOutBox);
        inOutBox->setDevice(dev);
    }
}