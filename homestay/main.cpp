#include "home_hotel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    home_hotel w;
    w.show();

    return a.exec();
}
