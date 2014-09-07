import qbs.base 1.0

DynamicLibrary {
    name: "qstorageinfo"
    destinationDirectory: project.install_library_path

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { condition:!qbs.targetOS.contains("windows"); name: "Qt.core-private" }

    cpp.defines: [ "QSTORAGEINFO_EXPORT=Q_DECL_EXPORT" ]

    Export {
        Depends { name: "cpp" }
        cpp.defines: [ "QSTORAGEINFO_EXPORT=Q_DECL_IMPORT" ]
    }

    files: [
        "qstorageinfo.cpp",
        "qstorageinfo.h",
        "qstorageinfo_p.h"
    ]

    Properties {
        condition: qbs.targetOS.contains("osx")
        cpp.frameworks: [ "CoreServices", "DiskArbitration" ]
        cpp.installNamePrefix: project.installNamePrefix
    }
    Properties {
         condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
         cpp.rpaths: "$ORIGIN"
    }
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [ "Netapi32", "Mpr", "user32", "Winmm" ]
    }

    Group {
        name: "mac"
        condition: qbs.targetOS.contains("osx")
        files: "qstorageinfo_mac.cpp"
    }
    Group {
        name: "unix"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        files: "qstorageinfo_unix.cpp"
    }
    Group {
        name: "windows"
        condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("wince")
        files: "qstorageinfo_win.cpp"
    }
    Group {
        name: "winrt"
        condition: qbs.targetOS.contains("winrt")
        files: "qstorageinfo_stub.cpp"
    }
    Group {
        name: "wince"
        condition: qbs.targetOS.contains("wince")
        files: "qstorageinfo_stub.cpp"
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.install_library_path
    }
}
