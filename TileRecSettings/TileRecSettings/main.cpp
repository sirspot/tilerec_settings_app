#include "TileRecSettings.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setApplicationName(QString("TileRec Settings"));
	QCoreApplication::setOrganizationDomain(QString("sirspot.com"));
	QCoreApplication::setOrganizationName(QString("sirspot.com"));

	QApplication a(argc, argv);
	TileRecSettings w;
	w.show();
	return a.exec();
}
