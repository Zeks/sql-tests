#include <gtest/gtest.h>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_context.h"
#include <QFileInfo>
#include <QFile>
#include <QDate>
#include <QSettings>


class DatabaseTestPQXX  : public ::testing::Test{
  protected:
  void SetUp() override {
      pgToken.tokenType = "PQXX";
      QSettings settings("postgres_coordinates.ini", QSettings::IniFormat);
      pgToken.ip = settings.value("test.postgres/hostname").toString().toStdString();
      pgToken.port= settings.value("test.postgres/port").toInt();
      pgToken.user = settings.value("test.postgres/user").toString().toStdString();
      pgToken.password = settings.value("test.postgres/pass").toString().toStdString();
  }
  void TearDown() override {
    sql::Database::removeDatabase("TestPostgresDatabase");
  }
  sql::ConnectionToken pgToken;
  std::string testDatabaseName = "PQXXDatabase";
};

// tests that adddatabase produces valid db driverType
TEST_F(DatabaseTestPQXX, DbAddTest){
    auto db = sql::Database::addDatabase("PQXX", testDatabaseName);
    EXPECT_EQ(db.driverType(), "PQXX");
}

// tests that constructors of the token type fill the necessary data
TEST_F(DatabaseTestPQXX, DbTokenTypeTest){

    sql::ConnectionToken pgtoken("testdb", "192.168.1.59", 3055, "socrates", "dummy");
    EXPECT_EQ(pgtoken.tokenType, "PQXX");
    EXPECT_EQ(pgtoken.serviceName, "testdb");
    EXPECT_EQ(pgtoken.ip, "192.168.1.59");
    EXPECT_EQ(pgtoken.port, 3055);
    EXPECT_EQ(pgtoken.user, "socrates");
    EXPECT_EQ(pgtoken.password, "dummy");
}

// ensuring  that trying to open the database without an assigned token returns a failure
TEST_F(DatabaseTestPQXX, DbOpenWithoutToken){
    auto db = sql::Database::addDatabase("PQXX", testDatabaseName);
    auto opened = db.open();
    EXPECT_EQ(opened, false);
}

TEST_F(DatabaseTestPQXX, DbOpenPqWithToken){
    auto db = sql::Database::addDatabase("PQXX", "TestPostgresDatabase");
    db.setConnectionToken(pgToken);
    auto opened = db.open();
    EXPECT_EQ(opened, true)  << "Couldn't open the sql database when it was supposed to open just fine from a token";
}

TEST_F(DatabaseTestPQXX, EnsureCreateDeleteTableWorksForPG){
    auto db = sql::Database::addDatabase("PQXX", "TestPostgresDatabase");
    db.setConnectionToken(pgToken);
    auto opened = db.open();
    EXPECT_EQ(opened, true)  << "Couldn't open the sql database when it was supposed to open just fine from a token";
    sql::Query q(db);
    q.prepare("create table if not exists tests.TEST_CREATE_TABLE(test_key varchar(6), test_value varchar(6))");
    auto result = q.exec();
    EXPECT_EQ(result, true);
}


TEST_F(DatabaseTestPQXX, EnsureSelectWorksForPG){
    auto db = sql::Database::addDatabase("PQXX", "TestPostgresDatabase");
    db.setConnectionToken(pgToken);
    auto opened = db.open();
    EXPECT_EQ(opened, true)  << "Couldn't open the sql database when it was supposed to open just fine from a token";
    sql::Query q(db);
    q.prepare("select * from tests.test_table_selects");
    // 1. ensure we have a record
    // 2. ensure all individual parts of the record can be read out.
    q.exec();
    auto fetchedNext = q.next();
    EXPECT_EQ(fetchedNext, true);
}


TEST_F(DatabaseTestPQXX, EnsureNamedValuesSelectWorksForPG){
    auto db = sql::Database::addDatabase("PQXX", "TestPostgresDatabase");
    db.setConnectionToken(pgToken);
    auto opened = db.open();
    EXPECT_EQ(opened, true)  << "Couldn't open the sql database when it was supposed to open just fine from a token";
    sql::Query q(db);
    q.prepare("select * from tests.test_table_selects");
    // 1. ensure we have a record
    // 2. ensure all individual parts of the record can be read out.
    q.exec();
    q.next();
    auto stringData = q.value("limited_text_data").toString();
    EXPECT_EQ(stringData.size() > 0, true) << "string is not supposed to be empty";
    EXPECT_EQ(stringData, "this is limited text");
    stringData = {};
    stringData = q.value("unlimited_text_data").toString();
    EXPECT_EQ(stringData.size() > 0, true) << "string is not supposed to be empty";
    auto intData = q.value("integer_data").toInt();
    EXPECT_EQ(intData == 42, true);
    auto dateData = q.value("date_data").toDate();
    auto testDate = QDate::fromString("1980-01-01", "yyyy-MM-dd");
    EXPECT_EQ(dateData == testDate, true);


}
