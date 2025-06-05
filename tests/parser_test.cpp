#include <gtest/gtest.h>
#include "sql/parser.h"

using namespace preql;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<sql::Parser>();
    }

    void TearDown() override {
        parser.reset();
    }

    std::unique_ptr<sql::Parser> parser;
};

TEST_F(ParserTest, CreateTable) {
    auto stmt = parser->parse("CREATE TABLE users (id INT, name VARCHAR, age INT)");
    ASSERT_TRUE(std::holds_alternative<sql::CreateTableStatement>(stmt));
    
    auto create_stmt = std::get<sql::CreateTableStatement>(stmt);
    EXPECT_EQ(create_stmt.table_name, "users");
    EXPECT_EQ(create_stmt.columns.size(), 3);
    
    EXPECT_EQ(create_stmt.columns[0].name, "id");
    EXPECT_EQ(create_stmt.columns[0].type, 0); // INT
    
    EXPECT_EQ(create_stmt.columns[1].name, "name");
    EXPECT_EQ(create_stmt.columns[1].type, 2); // VARCHAR
    
    EXPECT_EQ(create_stmt.columns[2].name, "age");
    EXPECT_EQ(create_stmt.columns[2].type, 0); // INT
}

TEST_F(ParserTest, Insert) {
    auto stmt = parser->parse("INSERT INTO users VALUES (1, 'John', 25)");
    ASSERT_TRUE(std::holds_alternative<sql::InsertStatement>(stmt));
    
    auto insert_stmt = std::get<sql::InsertStatement>(stmt);
    EXPECT_EQ(insert_stmt.table_name, "users");
    EXPECT_EQ(insert_stmt.values.size(), 3);
    EXPECT_EQ(insert_stmt.values[0], "1");
    EXPECT_EQ(insert_stmt.values[1], "John");
    EXPECT_EQ(insert_stmt.values[2], "25");
}

TEST_F(ParserTest, Select) {
    auto stmt = parser->parse("SELECT id, name FROM users WHERE age > 20");
    ASSERT_TRUE(std::holds_alternative<sql::SelectStatement>(stmt));
    
    auto select_stmt = std::get<sql::SelectStatement>(stmt);
    EXPECT_EQ(select_stmt.table_name, "users");
    EXPECT_EQ(select_stmt.columns.size(), 2);
    EXPECT_EQ(select_stmt.columns[0], "id");
    EXPECT_EQ(select_stmt.columns[1], "name");
    EXPECT_EQ(select_stmt.condition, "age > 20");
}

TEST_F(ParserTest, SelectAll) {
    auto stmt = parser->parse("SELECT * FROM users");
    ASSERT_TRUE(std::holds_alternative<sql::SelectStatement>(stmt));
    
    auto select_stmt = std::get<sql::SelectStatement>(stmt);
    EXPECT_EQ(select_stmt.table_name, "users");
    EXPECT_EQ(select_stmt.columns.size(), 1);
    EXPECT_EQ(select_stmt.columns[0], "*");
    EXPECT_TRUE(select_stmt.condition.empty());
}

TEST_F(ParserTest, Delete) {
    auto stmt = parser->parse("DELETE FROM users WHERE id = 1");
    ASSERT_TRUE(std::holds_alternative<sql::DeleteStatement>(stmt));
    
    auto delete_stmt = std::get<sql::DeleteStatement>(stmt);
    EXPECT_EQ(delete_stmt.table_name, "users");
    EXPECT_EQ(delete_stmt.condition, "id = 1");
}

TEST_F(ParserTest, Describe) {
    auto stmt = parser->parse("DESCRIBE users");
    ASSERT_TRUE(std::holds_alternative<sql::DescribeStatement>(stmt));
    
    auto describe_stmt = std::get<sql::DescribeStatement>(stmt);
    EXPECT_EQ(describe_stmt.table_name, "users");
}

TEST_F(ParserTest, InvalidSyntax) {
    EXPECT_THROW(parser->parse("CREATE TABLE"), std::runtime_error);
    EXPECT_THROW(parser->parse("INSERT INTO"), std::runtime_error);
    EXPECT_THROW(parser->parse("SELECT FROM"), std::runtime_error);
    EXPECT_THROW(parser->parse("DELETE FROM"), std::runtime_error);
    EXPECT_THROW(parser->parse("DESCRIBE"), std::runtime_error);
}

TEST_F(ParserTest, CaseInsensitive) {
    auto stmt1 = parser->parse("create table users (id int)");
    auto stmt2 = parser->parse("CREATE TABLE users (id INT)");
    EXPECT_EQ(stmt1.index(), stmt2.index());
    
    auto create1 = std::get<sql::CreateTableStatement>(stmt1);
    auto create2 = std::get<sql::CreateTableStatement>(stmt2);
    EXPECT_EQ(create1.table_name, create2.table_name);
    EXPECT_EQ(create1.columns.size(), create2.columns.size());
}

TEST_F(ParserTest, ComplexConditions) {
    auto stmt = parser->parse("SELECT * FROM users WHERE age > 20 AND name LIKE 'J%'");
    ASSERT_TRUE(std::holds_alternative<sql::SelectStatement>(stmt));
    
    auto select_stmt = std::get<sql::SelectStatement>(stmt);
    EXPECT_EQ(select_stmt.condition, "age > 20 AND name LIKE 'J%'");
}

TEST_F(ParserTest, ValidateStatements) {
    // Valid statements
    EXPECT_TRUE(parser->validate(parser->parse("CREATE TABLE users (id INT)")));
    EXPECT_TRUE(parser->validate(parser->parse("INSERT INTO users VALUES (1)")));
    EXPECT_TRUE(parser->validate(parser->parse("SELECT * FROM users")));
    EXPECT_TRUE(parser->validate(parser->parse("DELETE FROM users WHERE id = 1")));
    EXPECT_TRUE(parser->validate(parser->parse("DESCRIBE users")));
    
    // Invalid statements
    EXPECT_FALSE(parser->validate(parser->parse("CREATE TABLE")));
    EXPECT_FALSE(parser->validate(parser->parse("INSERT INTO users")));
    EXPECT_FALSE(parser->validate(parser->parse("SELECT FROM users")));
    EXPECT_FALSE(parser->validate(parser->parse("DELETE FROM users")));
    EXPECT_FALSE(parser->validate(parser->parse("DESCRIBE")));
} 