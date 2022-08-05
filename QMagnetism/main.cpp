#include "QMagnetism.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMagnetism w;
    w.show();
    return a.exec();
}
