TEMPLATE = lib
CONFIG += static
TARGET = qstorageinfo
DESTDIR = ../lib
QT = core core-private

CONFIG -= rtti exceptions
CONFIG += c++11
DEFINES *= QT_NO_CAST_FROM_BYTEARRAY QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

#HEADERS += qtdriveinfoglobal.h
HEADERS += qstorageinfo.h \
           qstorageinfo_p.h
SOURCES += qstorageinfo.cpp

win* {
    SOURCES += qstorageinfo_win.cpp
    LIBS += -lNetapi32 -lMpr -luser32 -lWinmm
}

unix {
    macx {
        SOURCES += qstorageinfo_mac.cpp
        LIBS += -framework CoreServices -framework DiskArbitration -framework IOKit
    } else {
        SOURCES += qstorageinfo_unix.cpp
    }
}
