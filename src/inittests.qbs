import qbs 1.0
import qbs.Process
import "../BaseDefines.qbs" as App
import "../Precompiled.qbs" as Precompiled

App{
    name: "tests"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    consoleApplication:false
    type:"application"
    Depends { name: "Qt.core"}

    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
    ]

    files: [
        "dummy_test.cpp",
        "gtest_main.cc"
    ]
    cpp.systemIncludePaths: [
        "/usr/src/googletest/googletest/include",
        "/usr/src/googletest/googletest/src"
        ]

    cpp.staticLibraries: {
        var libs = []
         libs = ["gtest_main", "gtest"]
        return libs
    }
}

