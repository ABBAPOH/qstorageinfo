TEMPLATE = app
QT += core gui widgets
DESTDIR = ../../bin

HEADERS += storagemodel.h
SOURCES += \
    storagemodel.cpp \
    main.cpp

LIBS += -L$$OUT_PWD/../../lib -lqstorageinfo
INCLUDEPATH += $$PWD/../../include

include(../../src/libs.pri)
