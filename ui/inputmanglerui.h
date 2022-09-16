/*
* Created by arek on 27.05.18.
*/

#pragma once

#include "ui_inputmanglerui.h"
#include <QMainWindow>
#include "imconfmodel.h"

class InputmanglerUI : public QMainWindow {
    Q_OBJECT

public:
    InputmanglerUI();


protected:
    Ui_InputmanglerUI *ui;
    IMConfModel *model;

};



