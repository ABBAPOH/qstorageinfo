win* {
    LIBS += -lNetapi32 -lMpr -luser32 -lWinmm
}

macx-* {
    LIBS += -framework CoreServices -framework DiskArbitration -framework IOKit
}

linux-*: {
}
