#include "TileRecSettings.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	TileRecSettings w;
	w.show();
	return a.exec();
}
