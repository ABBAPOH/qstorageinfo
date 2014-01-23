import qbs.base 1.0

Project {

    property string app_target: "andromeda-libs"
    property string install_binary_path: "bin"
    property string lib_suffix: ""
    property string install_library_path: {
        if (qbs.targetOS.contains("windows"))
            return install_app_path
        else
            return "lib" + lib_suffix
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
