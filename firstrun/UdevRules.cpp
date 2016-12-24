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

#include "UdevRules.h"
#include <QFile>
#include <QMap>

UdevRules::UdevRules(QString filename) {
    if (filename == "")
        filename = "/etc/udev/rules.d/80-inputmangler.rules";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QStringList parts = QString(file.readAll()).split("\n");

    Rule r;
    QMap<QString, Rule> sortedRules;
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (parts[i+1].contains("ATTRS")) {
            r.set(parts[i], parts[i+1]);
            r.lineNumber = i+1;
            if (!sortedRules.contains(r.name)) {
                rules.append(r);
                sortedRules[r.name] = r;
            }
            else {
                Rule rr = sortedRules[r.name];
                if (rr.product != r.product || rr.vendor != r.vendor) {
                    rr.verboseName = sortedRules[r.name].verboseName = true;
                    rules.append(r);
                    sortedRules[r.name] = r;
                }
                else if (rr.active == true &&  r.active == false) {
                    sortedRules[r.name] = rr;
                }
            }
        }
    }
}

void UdevRules::Rule::set(QString line1, QString line2) {
    name = line1.mid(2, line1.size() - 3);

    active = !line2.startsWith('#');
    QStringList list = line2.split(',');
    Q_FOREACH (QString s, list) {
      if (s.contains("idVendor"))
        vendor = s.mid(s.indexOf("==\"") + 3, 4);
      else if (s.contains("idProduct"))
        product = s.mid(s.indexOf("==\"") + 3, 4);
    }
}

QString UdevRules::Rule::getName() {
    if (!verboseName)
        return name;
    return name + " (vendor: " + vendor + ", product: " + product + ")";
}

QString UdevRules::Rule::applyCommand(bool newState) {
    if (newState == active)
        return "";
    if (newState)
        return "sed -i'' '" + lineNumber + "s/#//' /tmp/80-inputmangler.rules";
    return "sed -i'' '" + lineNumber + "s/^/#/' /tmp/80-inputmangler.rules";
   //     return "sed -i'' '" + lineNumber + "s/#//' /etc/udev.d/80-inputmangler.rules";
    //return "sed -i'' '" + lineNumber + "s/^/#/' /etc/udev.d/80-inputmangler.rules";
}
