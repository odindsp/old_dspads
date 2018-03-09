#include <QApplication>
#include "Amax_Test.h"
#include "Thread.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Amax_Test w;
    w.show();
    return a.exec();
}
