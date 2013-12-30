TEMPLATE = lib
CONFIG += static
TARGET = qdriveinfo
DESTDIR = ../lib
QT = core core-private

CONFIG -= rtti exceptions
DEFINES *= QT_NO_CAST_FROM_BYTEARRAY QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

#HEADERS += qtdriveinfoglobal.h
HEADERS += qdriveinfo.h \
           qdriveinfo_p.h
SOURCES += qdriveinfo.cpp

win* {
    SOURCES += qdriveinfo_win.cpp
    LIBS += -luserenv -lNetapi32 -lMpr -luser32 -lWinmm
}

unix {
    macx {
        SOURCES += qdriveinfo_mac.cpp
        LIBS += -framework CoreServices -framework DiskArbitration -framework IOKit
    } else {
        SOURCES += qdriveinfo_unix.cpp
    }
}
