TEMPLATE = app
QT += core gui widgets
DESTDIR = ../../bin

HEADERS += drivemodel.h
SOURCES += \
    drivemodel.cpp \
    main.cpp

LIBS += -L$$OUT_PWD/../../lib -lqdriveinfo
INCLUDEPATH += $$PWD/../../include

include(../../src/libs.pri)
