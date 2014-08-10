TEMPLATE = app
QT += core testlib
CONFIG -= app_bundle
CONFIG += console

SOURCES += tst_qstorageinfo.cpp
INCLUDEPATH += $$PWD/../../../include
LIBS += -L$$OUT_PWD/../../../lib -lqstorageinfo

include($$PWD/../../../src/libs.pri)
