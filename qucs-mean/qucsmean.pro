QT += core gui widgets

CONFIG += c++17

TARGET = qucsmean
TEMPLATE = app

SOURCES += main.cpp \
           meanwindow.cpp

HEADERS += meanwindow.h

# Default install path
target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
