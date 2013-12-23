TEMPLATE = app
QT += core testlib
CONFIG -= app_bundle
CONFIG += console

SOURCES += tst_qdriveinfo.cpp
INCLUDEPATH += $$PWD/../../../include
LIBS += -L$$OUT_PWD/../../../lib -lqdriveinfo

include($$PWD/../../../src/libs.pri)
