TEMPLATE = app
QT += core testlib
CONFIG -= app_bundle
CONFIG += console

SOURCES += tst_qvolumeinfo.cpp
INCLUDEPATH += $$PWD/../../../include
LIBS += -L$$OUT_PWD/../../../lib -lqvolumeinfo

include($$PWD/../../../src/libs.pri)
