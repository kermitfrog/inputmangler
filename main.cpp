#include "inputmangler.h"
#include "qtsignalhandler.h"
#include <QCoreApplication>


int main(int argc, char **argv) {
	QCoreApplication a(argc,argv);
	InputMangler im;
	QtSignalHandler s;
	QObject::connect(&a, SIGNAL(aboutToQuit()), &im, SLOT(cleanUp()));
	a.exec();
    return 0;
}
