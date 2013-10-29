#include "inputmangler.h"
#include "qtsignalhandler.h"
#include <QCoreApplication>
#include "imdbusinterface.h"


int main(int argc, char **argv) {
	QCoreApplication a(argc,argv);
	imDbusInterface dbus;	//has, for some reason, to be constructed in main
	InputMangler im;
	QtSignalHandler s;		//handles TERM and HUP
	QObject::connect(&dbus, SIGNAL(windowChanged(QString)), &im, SLOT(activeWindowChanged(QString)));
	QObject::connect(&dbus, SIGNAL(windowTitleChanged(QString)), &im, SLOT(activeWindowTitleChanged(QString)));
	QObject::connect(&s, SIGNAL(hupReceived()), &im, SLOT(reReadConfig()));
	QObject::connect(&a, SIGNAL(aboutToQuit()), &im, SLOT(cleanUp()));
	a.exec();
    return 0;
}
