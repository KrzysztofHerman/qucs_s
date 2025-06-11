#include <QApplication>
#include "meanwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MeanWindow window;
    window.show();
    return app.exec();
}
