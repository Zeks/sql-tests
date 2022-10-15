import qbs 1.0
import qbs.File
Project {
    name: "sql_tests"
    property bool usePostgres: true
    property string rootFolder: {
        var rootFolder = File.canonicalFilePath(sourceDirectory).toString();
        console.error("Source:" + rootFolder)
        return rootFolder.toString()
    }
    references: [
        "test_product.qbs",
        "core_condition.qbs",
        "environment_plugs.qbs",
        "libs/Logger/logger.qbs",
        "libs/sql/sql.qbs",
    ]
}

