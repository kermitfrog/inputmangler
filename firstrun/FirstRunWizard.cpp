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

#include "FirstRunWizard.h"
#include <QProcess>
#include <QFile>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QListWidget>

FirstRunWizard::FirstRunWizard() {
    ui = new Ui_Wizard();
    ui->setupUi(this);

    step1();
    step2();
}

void FirstRunWizard::step1() {
    QTreeWidgetItem *group;
    QTreeWidgetItem *it;
    QString tmp;

    group = addTreeItem(tr("kernel module"));
    checkItems[kmInstalled] = addTreeItem(tr("installed"), group, tr(""));
    checkItems[kmAtStart] = addTreeItem(tr("loads at system start"), group, tr(""));
    checkItems[kmLoaded] = addTreeItem(tr("loaded"), group, tr(""));
    group->setExpanded(true);

    group = addTreeItem(tr("UDev"));
    checkItems[udevKMRules] = addTreeItem(tr("kernel module rules"), group, tr(""));
    checkItems[udevDevRules] = addTreeItem(tr("device rules created"), group, tr(""));
    checkItems[udevActive] = addTreeItem(tr("active now"), group, tr(""));
    group->setExpanded(true);

    /// check
    tmp = runCommandSimple("uname -r");
    tmp = runCommandSimple("find /lib/modules/" + tmp.trimmed() + " -name inputdummy.ko");
    setChecked (kmInstalled, (tmp.trimmed().size() > 0));

    setChecked(kmAtStart, (runCommandSimple("grep -Re ^inputdummy$ /etc/modules /etc/modules-load.d/").contains("inputdummy")));

    setChecked(kmLoaded, runCommandSimple("lsmod").contains("\ninputdummy "));

    setChecked(udevKMRules, QFile::exists("/lib/udev/rules.d/40-inputdummy.rules"));

    setChecked(udevDevRules, QFile::exists("/etc/udev/rules.d/80-inputmangler.rules"));

    setChecked(udevActive, runCommandSimple("ls -l /dev/input").contains(groupName)
                           && runCommandSimple("ls -l /dev/virtual_kbd").contains(groupName) );
}

void FirstRunWizard::step2() {
    udevRules = new UdevRules();
    for (int i = 0; i < udevRules->rules.size(); ++i) {
        UdevRules::Rule rule = udevRules->rules[i];
        QListWidgetItem *it = new QListWidgetItem(rule.getName(), ui->deviceListWidget);
        it->setCheckState(boolToCheck(rule.active));
    }


}

QTreeWidgetItem* FirstRunWizard::addTreeItem(QString text, QTreeWidgetItem *parent, QString description) {
    QTreeWidgetItem *it;
    if (parent != nullptr) {
        it = new QTreeWidgetItem(1);
        it->setCheckState(0, Qt::CheckState::Checked);
        parent->addChild(it);
    }
    else {
        it = new QTreeWidgetItem(0);
        ui->tree->addTopLevelItem(it);
    }
    it->setText(0, text);
    it->setStatusTip(0, description);
    return it;

}

QString FirstRunWizard::runCommandSimple(QString cmd) {
    QProcess proc;
    proc.start(cmd);
    proc.waitForFinished(300000);
    QString output = QString(proc.readAllStandardOutput());
    return output;
}

QString FirstRunWizard::runCommandSudo(QString cmd) {
    if (cmd.isEmpty())
        return "";
    QProcess proc;
    QString output;
    bool passRequired = false;
    proc.start("sudo " + cmd);
    for (int i = 0; i < 10; i++) {
        proc.waitForBytesWritten();
        output += QString(proc.readAllStandardOutput());
        if (output.contains("[sudo] password")) {
            passRequired = true;
            break;
        } else if (output.contains("\n"))
            break;
    }

    if (passRequired)
        if (password.isEmpty()) {
            do {
                password = QInputDialog::getText(nullptr, "Enter password for sudo", "Password",
                                                 QLineEdit::EchoMode::Password);
                proc.write(password.toLocal8Bit().data());
                proc.waitForBytesWritten();
                output = proc.readAllStandardOutput();
            } while (output.contains("Sorry, try again."));
        } else {
            proc.write(password.toLocal8Bit().data());
        }
    proc.waitForFinished();
    output = proc.readAllStandardOutput();
    return output;
}

