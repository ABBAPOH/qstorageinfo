import qbs.base 1.0

Project {

    property string app_target: "qstorageinfo"
    property string install_binary_path: "bin"
    property string install_library_path: {
        if (qbs.targetOS.contains("windows"))
            return "bin";
        return "lib"
    }

    property string installNamePrefix: "@executable_path/../" + install_library_path + "/"

    SubProject {
        filePath: "examples/examples.qbs"
    }
    SubProject {
        filePath: "src/src.qbs"
    }
    SubProject {
        filePath: "tests/tests.qbs"
    }
}
