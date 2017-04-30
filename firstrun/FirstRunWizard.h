/*  
    Created by arek on 2016-12-10.
    
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

#ifndef INPUTMANGLER_FIRSTRUNWIZARD_H
#define INPUTMANGLER_FIRSTRUNWIZARD_H

#include <QtWidgets>
#include <QtWidgets/QTreeWidget>
#include "ui_firstrun.h"
#include "UdevRules.h"

class FirstRunWizard : public QWizard {
    Q_OBJECT

    enum PreCheckId {kmInstalled = 0, kmAtStart = 1, kmLoaded = 2, udevKMRules = 3, udevDevRules = 4, udevActive = 5};
public:
    FirstRunWizard();
    const QString groupName = "inputmangler";
    const QString mapPath = "/usr/share/inputmangler/keymaps/";

public slots:
    void setShowSystemUsers(int show);

protected:
    Ui_Wizard *ui;
    UdevRules *udevRules;
    void step1();
    void step2();
    void step3();
    void step4();
    void step5();
    void step6();
    QTreeWidgetItem* addTreeItem(QString text, QTreeWidgetItem *parent = nullptr, QString description = "");
    QTreeWidgetItem* checkItems[6];
    QString runCommandSimple(QString cmd);
    QString runCommandSudo(QString cmd);
    QStringList contentsOf(QString filename);
    void setChecked(PreCheckId id, bool value) {
        checkItems[id]->setCheckState(0, value ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);};
    Qt::CheckState boolToCheck(bool value) { return value ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;};
    bool groupExists = false;
    QStringList usersInGroup;

private:
    QString user;
    QString password;

};



#endif //INPUTMANGLER_FIRSTRUNWIZARD_H
