import qbs.base 1.0

Product {
    type: "application"
    name: "volumeview"
    destinationDirectory: project.install_binary_path

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "QStorageInfo" }

    cpp.includePaths: "../../include"

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        cpp.rpaths: [ "$ORIGIN/../lib" + project.lib_suffix ]
    }

    files: [
        "main.cpp",
        "storagemodel.cpp",
        "storagemodel.h"
    ]

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.install_binary_path
    }
}
