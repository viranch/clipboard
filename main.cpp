#include <QtGui/QApplication>
#include "clipboard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClipBoard w;
    w.show();

    return a.exec();
}
