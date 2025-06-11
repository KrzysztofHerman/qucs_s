#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <QtCore>
#include <QtWidgets>
#include <QApplication>

#include "qucstouchstoneviewer.h"

struct tQucsSettings QucsSettings; // Assuming this is a global or accessible struct

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Basic settings (can be expanded later)
    QucsSettings.x = 200;
    QucsSettings.y = 100;

    QucsTouchstoneViewer *w = new QucsTouchstoneViewer();
    w->raise();
    w->move(QucsSettings.x, QucsSettings.y);
    w->show();

    int result = a.exec();
    // Optionally save settings here if needed
    return result;
}
