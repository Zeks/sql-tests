#include <gtest/gtest.h>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_error.h"
#include <QFileInfo>
#include <QFile>
#include <QDate>
#include <QSettings>


struct TestTableData{
    TestTableData(int key,std::string value,QDateTime date):key(key), value(value), date(date){}
    bool operator==(const TestTableData& other) const{
        return this->key == other.key &&
                this->value == other.value &&
                this->date == other.date;
    };
    int key;
    std::string value;
    QDateTime date;
};

class QueryTestsSqlite: public ::testing::Test{
protected:
    QueryTestsSqlite(){
        sqliteToken = sql::ConnectionToken ("testdb.sqlite", "test_db_init.sql", "");
        db = sql::Database::addDatabase("QSQLITE", testDatabaseName);;
        db.setConnectionToken(sqliteToken);
        db.open();
        testTableData.emplace_back(1, "some value", QDateTime::fromString("1981-07-22 09:00", "yyyy-MM-dd mm:ss"));
        testTableData.emplace_back(2, "some other value", QDateTime::fromString("1981-07-21 09:00", "yyyy-MM-dd mm:ss"));
        CreateDatabaseForTesting();
    }

    void TearDown() override {
        db = {};
        sql::Database::removeDatabase(testDatabaseName);
        QFile::remove(QString::fromStdString(sqliteDatabaseFile));
    }
    void CreateDatabaseForTesting();

    std::string sqliteDatabaseFile = "testdb.sqlite";
    std::string testDatabaseName = "TestSqliteDatabase";
    sql::ConnectionToken sqliteToken;
    sql::Database db;
    std::vector<TestTableData> testTableData;
};


TEST_F(QueryTestsSqlite, EmptyObjectTest){
    // initialization with null database
    sql::Database db;
    sql::Query q(db);
    // should throw an exception on access
    EXPECT_THROW(q.lastQuery(), std::logic_error);
    EXPECT_THROW(q.lastError(), std::logic_error);
    EXPECT_THROW(q.value(0), std::logic_error);
    EXPECT_THROW(q.value(""), std::logic_error);
    EXPECT_THROW(q.supportsImmediateResultSize(), std::logic_error);
    EXPECT_THROW(q.rowCount(), std::logic_error);
    EXPECT_THROW(q.next(), std::logic_error);
    EXPECT_THROW(q.exec(), std::logic_error);
}

TEST_F(QueryTestsSqlite, ValidDBNoQueryObjectTest){
    sql::Query q(db);
    // calling next() on an empty object should always return false
    // calling exec with an empty object should return false
    EXPECT_TRUE(q.lastQuery() == "");
    EXPECT_TRUE(q.lastError() == sql::Error::noError());

    // qt sqlite doesn't fetch full result size by default
    // as a result it should always return rowcount = 0
    EXPECT_FALSE(q.supportsImmediateResultSize());
    EXPECT_EQ(q.rowCount() == 0, true);

    // calling exec with an empty object should return false
    EXPECT_FALSE(q.exec());
    // calling next() on an empty object should always return false
    EXPECT_FALSE(q.next());

}


TEST_F(QueryTestsSqlite, ValidPrepare){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("select 42 as meaning"));
    EXPECT_TRUE(q.lastError() == sql::Error::noError());
}

TEST_F(QueryTestsSqlite, InvalidPrepare){
    sql::Query q(db);
    EXPECT_FALSE(q.prepare("@#$^@#$"));
    auto error = q.lastError();
    EXPECT_FALSE(error == sql::Error::noError());
    EXPECT_TRUE(error.getActualErrorType() == sql::ESqlErrors::se_generic_sql_error);
}

TEST_F(QueryTestsSqlite, DirectDDLStatement){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("create table if not exists test_ddl_table(id integer)"));
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    EXPECT_TRUE(error == sql::Error::noError());
}


TEST_F(QueryTestsSqlite, SelectWithoutBindsStatement){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("select * from TEST_CREATE_TABLE"));
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    EXPECT_TRUE(error == sql::Error::noError());

    // sqlite doesn't support it
    EXPECT_FALSE(q.rowCount());
    EXPECT_TRUE(q.next());
    EXPECT_TRUE(q.next());
    // we only inserted two rows so this should go outside the resultset and fail
    EXPECT_FALSE(q.next());
    // and obviously subsequent invocations should return false
    EXPECT_FALSE(q.next());
    // resetting the query
    EXPECT_TRUE(q.exec());

    int i = 0;
    while(q.next()){
        auto data = TestTableData{q.value("test_key").toInt(),
                     q.value("test_string_value").toString(),
                                   q.value("test_date_value").toDateTime()};
        EXPECT_TRUE(data == testTableData[i]);
        i++;
    }
}

TEST_F(QueryTestsSqlite, PositionalValueAccess){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("select * from TEST_CREATE_TABLE"));
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    EXPECT_TRUE(error == sql::Error::noError());
    int i = 0;
    while(q.next()){
        auto data = TestTableData{q.value(0).toInt(),
                     q.value(1).toString(),
                                   q.value(2).toDateTime()};
        EXPECT_TRUE(data == testTableData[i]);
        i++;
    }
}

// do invalid query like no such column
TEST_F(QueryTestsSqlite, WhereWithBindsStatement){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("select * from TEST_CREATE_TABLE where test_key = :id"));
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    // we didn't assign a required binding but sqlite doesn't check anything like that
    EXPECT_TRUE(error == sql::Error::noError());
    // obviously we shouldn't be selecting anything with this
    EXPECT_FALSE(q.next());
    q.bindValue(":id", 1);
    EXPECT_TRUE(q.exec());
    // this is not a correct binding syntax, use without :
    EXPECT_FALSE(q.next());

    q.bindValue("id", 1);
    EXPECT_TRUE(q.exec());
    // this is not a correct binding syntax, use without :
    EXPECT_TRUE(q.next());

    auto data = TestTableData{q.value("test_key").toInt(),
                 q.value("test_string_value").toString(),
                               q.value("test_date_value").toDateTime()};
    // if we succeeded - binding works
    EXPECT_TRUE(data == testTableData[0]);
}


TEST_F(QueryTestsSqlite, VerifyLastQuery){
    sql::Query q(db);
    std::string query = "select * from TEST_CREATE_TABLE";
    EXPECT_TRUE(q.prepare(query));
    EXPECT_TRUE(q.exec());
    EXPECT_EQ(q.lastQuery(), query);

    EXPECT_FALSE(q.prepare(""));
    EXPECT_EQ(q.lastQuery(), "");
}

TEST_F(QueryTestsSqlite, ResetOfExec){
    sql::Query q(db);
    EXPECT_TRUE(q.prepare("select * from TEST_CREATE_TABLE"));
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    EXPECT_TRUE(error == sql::Error::noError());
    EXPECT_FALSE(q.prepare(""));
    // expecting it to fail on empty query
    EXPECT_FALSE(q.exec());
    EXPECT_FALSE(q.next());
    EXPECT_TRUE(q.lastError().isValid());
    EXPECT_EQ(q.lastQuery() == "", true) << q.lastQuery();
}


TEST_F(QueryTestsSqlite, AlternativeConstructor){
    sql::Query q("select * from TEST_CREATE_TABLE", db);
    EXPECT_TRUE(q.exec());
    auto error = q.lastError();
    EXPECT_FALSE(error.isValid());
    // this is not a correct binding syntax, use without :
    EXPECT_TRUE(q.next());
    auto data = TestTableData{q.value("test_key").toInt(),
                 q.value("test_string_value").toString(),
                               q.value("test_date_value").toDateTime()};
    EXPECT_TRUE(data == testTableData[0]);
}

void QueryTestsSqlite::CreateDatabaseForTesting()
{
    sql::Query q(db);
    q.prepare("create table if not exists TEST_CREATE_TABLE(test_key integer, test_string_value varchar, test_date_value varchar)");
    q.exec();
    q.lastError();

    q.prepare("Insert into TEST_CREATE_TABLE(test_key, test_string_value, test_date_value)  values(:key, :value, :date)");
    for(const auto& data: testTableData){
        q.bindValue("key", data.key);
        q.bindValue("value", data.value);
        q.bindValue("date", data.date);
        q.exec();
        sql::Error error;
        error = q.lastError();
        error = q.lastError();
    }
}
