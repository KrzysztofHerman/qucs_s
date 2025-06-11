#include <QApplication>
#include "qucsmeanwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QucsMeanWindow window;
    window.show();
    return app.exec();
}
