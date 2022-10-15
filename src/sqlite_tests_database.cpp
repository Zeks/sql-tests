#include <gtest/gtest.h>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_context.h"
#include <QFileInfo>
#include <QFile>
#include <QDate>
#include <QSettings>


class DatabaseBasicTestsSqlite: public ::testing::Test{
protected:
    DatabaseBasicTestsSqlite(){
        sqliteToken = sql::ConnectionToken ("testdb.sqlite", "test_db_init.sql", "");
    }

    void TearDown() override {
        sql::Database::removeDatabase(testDatabaseName);
        QFile::remove(QString::fromStdString(sqliteDatabaseFile));
    }
    std::string sqliteDatabaseFile = "testdb.sqlite";
    std::string testDatabaseName = "TestSqliteDatabase";
    sql::ConnectionToken sqliteToken;
};



class DatabaseTestSqlite: public DatabaseBasicTestsSqlite{
protected:
    void SetUp() override {
        sqliteToken = sql::ConnectionToken ("testdb.sqlite", "test_db_init.sql", "");
        db = sql::Database::addDatabase("QSQLITE", testDatabaseName);;
        db.setConnectionToken(sqliteToken);
        db.open();
    }

    void TearDown() override {
        db = {};
        sql::Database::removeDatabase(testDatabaseName);
        QFile::remove(QString::fromStdString(sqliteDatabaseFile));
    }
    sql::Database db;
};




// tests that adddatabase produces valid db driverType
TEST_F(DatabaseBasicTestsSqlite, DbAccessTest){
    sql::Database db;

    auto ensureNoDatabase = [&](){
        db = sql::Database::database(testDatabaseName);
        EXPECT_TRUE(db.driverType() == "null");
        EXPECT_FALSE(db.isOpen());
    };

    //initially, we don't know anything and it will be invalid database
    ensureNoDatabase();

    db = sql::Database::addDatabase("QSQLITE", testDatabaseName);
    EXPECT_EQ(db.driverType(), "QSQLITE");
    EXPECT_EQ(db.connectionName(), QString::fromStdString(testDatabaseName).toUpper().toStdString());
    EXPECT_FALSE(db.isOpen());
    EXPECT_FALSE(db.hasOpenTransaction());
    EXPECT_TRUE(db.internalPointer());
    db = {};
    sql::Database::removeDatabase(testDatabaseName);
    ensureNoDatabase();
}

// tests that constructors of the token type fill the necessary data
TEST_F(DatabaseBasicTestsSqlite, DbTokenTypeTest){
    sql::ConnectionToken sqliteToken("testdb", "test_db_init.sql", "");
    EXPECT_EQ(sqliteToken.tokenType, "QSQLITE");
    EXPECT_EQ(sqliteToken.serviceName, "testdb");
    EXPECT_EQ(sqliteToken.initFileName, "test_db_init.sql");
    EXPECT_EQ(sqliteToken.ip, "");
    EXPECT_EQ(sqliteToken.port, 0);
    EXPECT_EQ(sqliteToken.user, "");
    EXPECT_EQ(sqliteToken.password, "");

}

// ensuring  that trying to open the database without an assigned token returns a failure
TEST_F(DatabaseBasicTestsSqlite, DbOpenWithoutToken){
    auto db = sql::Database::addDatabase("QSQLITE", testDatabaseName);
    auto opened = db.open();
    EXPECT_EQ(opened, false);
}

TEST_F(DatabaseBasicTestsSqlite, DbOpenSqliteWithToken){
    auto db = sql::Database::addDatabase("QSQLITE", testDatabaseName);
    db.setConnectionToken(sqliteToken);
    auto opened = db.open();
    EXPECT_EQ(opened, true)  << "Couldn't open the sql database when it was supposed to open just fine from a token and a file";
    EXPECT_TRUE(db.isOpen());
    bool hasFile = QFileInfo::exists(QString::fromStdString(sqliteDatabaseFile));
    EXPECT_EQ(hasFile, true)  << "wasn't able to find a file that should have been created";
    db.close();
    EXPECT_FALSE(db.isOpen());
}


TEST_F(DatabaseTestSqlite, DbPerformTransaction){
    sql::Query q(db);
    q.prepare("create table if not exists TEST_CREATE_TABLE(test_key integer, test_string_value varchar, test_date_value varchar)");
    auto result = q.exec();
    auto error = q.lastError();
    EXPECT_EQ(result, true);

    auto testDate = QDateTime::fromString("1981-07-21 09:00", "yyyy-MM-dd mm:ss");
    auto fillWithData = [&](){
        q.prepare("Insert into TEST_CREATE_TABLE(test_key, test_string_value, test_date_value)  values(:key, :value, :date)");
        q.bindValue("key", 1);
        q.bindValue("value", "some value");
        q.bindValue("date", testDate);
        result = q.exec();
        error = q.lastError();
    };
    auto readAvailable = [&]()-> bool{
        q.prepare("SELECT test_key, test_string_value, test_date_value FROM TEST_CREATE_TABLE");
        q.exec();
        return q.next();
    };

    EXPECT_FALSE(db.hasOpenTransaction());
    db.transaction();
    EXPECT_TRUE(db.hasOpenTransaction());
    fillWithData();
    db.rollback();
    EXPECT_FALSE(db.hasOpenTransaction());
    result = readAvailable();
    EXPECT_EQ(result, false);

    db.transaction();
    fillWithData();
    db.commit();
    EXPECT_FALSE(db.hasOpenTransaction());

    result = readAvailable();
    EXPECT_EQ(result, true);
    EXPECT_EQ(q.value("test_key").toInt() == 1, true) << "actually received: " << q.value("test_key").toInt() ;
    EXPECT_EQ(q.value("test_string_value").toString() == "some value", true)<< "actually received: " << q.value("test_value").toString() ;;
    EXPECT_EQ(q.value("test_date_value").toDateTime() == testDate , true)<< "actually received: " << q.value("test_date").toDateTime().toString().toStdString();

    db.transaction();
    EXPECT_TRUE(db.hasOpenTransaction());
    db.close();
    EXPECT_FALSE(db.hasOpenTransaction());
    EXPECT_FALSE(db.isOpen());


}
