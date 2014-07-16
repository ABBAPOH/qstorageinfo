TEMPLATE = lib
CONFIG += static
TARGET = qvolumeinfo
DESTDIR = ../lib
QT = core core-private

CONFIG -= rtti exceptions
CONFIG += c++11
DEFINES *= QT_NO_CAST_FROM_BYTEARRAY QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

#HEADERS += qtdriveinfoglobal.h
HEADERS += qvolumeinfo.h \
           qvolumeinfo_p.h
SOURCES += qvolumeinfo.cpp

win* {
    SOURCES += qvolumeinfo_win.cpp
    LIBS += -luserenv -lNetapi32 -lMpr -luser32 -lWinmm
}

unix {
    macx {
        SOURCES += qvolumeinfo_mac.cpp
        LIBS += -framework CoreServices -framework DiskArbitration -framework IOKit
    } else {
        SOURCES += qvolumeinfo_unix.cpp
    }
}
