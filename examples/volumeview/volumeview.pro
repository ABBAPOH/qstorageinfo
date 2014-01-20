TEMPLATE = app
QT += core gui widgets
DESTDIR = ../../bin

HEADERS += volumemodel.h
SOURCES += \
    volumemodel.cpp \
    main.cpp

LIBS += -L$$OUT_PWD/../../lib -lqvolumeinfo
INCLUDEPATH += $$PWD/../../include

include(../../src/libs.pri)
