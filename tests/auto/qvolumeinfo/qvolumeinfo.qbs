import qbs.base 1.0

Product {
    type: "application"
    destinationDirectory: project.install_binary_path

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.test" }
    Depends { name: "QVolumeInfo" }

    cpp.includePaths: "../../../include"

    files: "tst_qvolumeinfo.cpp"

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.install_binary_path
    }
}
