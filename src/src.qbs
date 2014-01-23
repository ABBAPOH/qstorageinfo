import qbs.base 1.0

DynamicLibrary {
    name: "QVolumeInfo"
    destinationDirectory: project.install_library_path

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private" }

    files: [
        "qvolumeinfo.cpp",
        "qvolumeinfo.h",
        "qvolumeinfo_p.h"
    ]

    Properties {
        condition: qbs.targetOS.contains("osx")
        cpp.frameworks: [ "CoreServices", "DiskArbitration", "IOKit" ]
        cpp.installNamePrefix: project.installNamePrefix
    }
    Properties {
         condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
         cpp.rpaths: "$ORIGIN"
    }
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [ "userenv", "Netapi32", "Mpr", "user32", "Winmm" ]
    }

    Group {
        name: "mac"
        condition: qbs.targetOS.contains("osx")
        files: "qvolumeinfo_mac.cpp"
    }
    Group {
        name: "unix"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        files: "qvolumeinfo_unix.cpp"
    }
    Group {
        name: "windows"
        condition: qbs.targetOS.contains("windows")
        files: "qvolumeinfo_win.cpp"
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.install_library_path
    }
}
