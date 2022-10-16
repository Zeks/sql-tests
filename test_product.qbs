import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App
import "Precompiled.qbs" as Precompiled

App{
    name: "tests"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    consoleApplication:false
    type:"application"
    Depends { name: "Qt.core"}
    Depends { name: "sql_abstractions"}

    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/include",
    ]

    files: [
        "db_scripts.qrc",
        "src/gtest_main.cc",
        "src/pqxx_tests_database.cpp",
        "src/pqxx_tests_query.cpp",
        "src/sqlite_tests_database.cpp",
        "src/sqlite_tests_query.cpp",
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


