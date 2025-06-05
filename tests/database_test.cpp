#include <gtest/gtest.h>
#include "core/database.h"
#include <filesystem>
#include <fstream>

using namespace preql;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test database
        db = std::make_unique<core::Database>();
        db->create("test_db");
    }

    void TearDown() override {
        // Clean up test database
        if (db->isOpen()) {
            db->close();
        }
        db->drop("test_db");
        db.reset();
    }

    std::unique_ptr<core::Database> db;
};

TEST_F(DatabaseTest, CreateAndDropDatabase) {
    EXPECT_TRUE(db->isOpen());
    db->close();
    EXPECT_FALSE(db->isOpen());
    
    // Test creating a new database
    EXPECT_TRUE(db->create("new_db"));
    EXPECT_TRUE(db->isOpen());
    
    // Test dropping database
    EXPECT_TRUE(db->drop("new_db"));
    EXPECT_FALSE(db->isOpen());
}

TEST_F(DatabaseTest, CreateAndDropTable) {
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},  // INT
        {"name", 2}, // VARCHAR
        {"age", 0}   // INT
    };
    
    EXPECT_TRUE(db->createTable("users", columns));
    EXPECT_TRUE(db->dropTable("users"));
}

TEST_F(DatabaseTest, InsertAndSelect) {
    // Create table
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},    // INT
        {"name", 2},  // VARCHAR
        {"age", 0}    // INT
    };
    EXPECT_TRUE(db->createTable("users", columns));
    
    // Insert data
    std::vector<std::string> values = {"1", "John", "25"};
    EXPECT_TRUE(db->insert("users", values));
    
    // Select data
    std::vector<std::string> selected_columns = {"id", "name", "age"};
    std::vector<std::vector<std::string>> results;
    
    EXPECT_TRUE(db->select("users", selected_columns, "", 
        [&results](const std::vector<std::string>& row) {
            results.push_back(row);
        }));
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0][0], "1");
    EXPECT_EQ(results[0][1], "John");
    EXPECT_EQ(results[0][2], "25");
}

TEST_F(DatabaseTest, DeleteRecords) {
    // Create table
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},    // INT
        {"name", 2},  // VARCHAR
        {"age", 0}    // INT
    };
    EXPECT_TRUE(db->createTable("users", columns));
    
    // Insert multiple records
    std::vector<std::string> values1 = {"1", "John", "25"};
    std::vector<std::string> values2 = {"2", "Jane", "30"};
    EXPECT_TRUE(db->insert("users", values1));
    EXPECT_TRUE(db->insert("users", values2));
    
    // Delete record with condition
    EXPECT_TRUE(db->delete_("users", "id = 1"));
    
    // Verify deletion
    std::vector<std::string> selected_columns = {"id", "name", "age"};
    std::vector<std::vector<std::string>> results;
    
    EXPECT_TRUE(db->select("users", selected_columns, "", 
        [&results](const std::vector<std::string>& row) {
            results.push_back(row);
        }));
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0][0], "2");
    EXPECT_EQ(results[0][1], "Jane");
    EXPECT_EQ(results[0][2], "30");
}

TEST_F(DatabaseTest, DescribeTable) {
    // Create table
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},    // INT
        {"name", 2},  // VARCHAR
        {"age", 0}    // INT
    };
    EXPECT_TRUE(db->createTable("users", columns));
    
    // Describe table
    EXPECT_TRUE(db->describe("users"));
}

TEST_F(DatabaseTest, ListTables) {
    // Create multiple tables
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},    // INT
        {"name", 2}   // VARCHAR
    };
    
    EXPECT_TRUE(db->createTable("users", columns));
    EXPECT_TRUE(db->createTable("products", columns));
    
    // List tables
    std::vector<std::string> tables = db->listTables();
    ASSERT_EQ(tables.size(), 2);
    EXPECT_TRUE(std::find(tables.begin(), tables.end(), "users") != tables.end());
    EXPECT_TRUE(std::find(tables.begin(), tables.end(), "products") != tables.end());
}

TEST_F(DatabaseTest, InvalidOperations) {
    // Try to create table in closed database
    db->close();
    std::vector<std::pair<std::string, int>> columns = {
        {"id", 0},    // INT
        {"name", 2}   // VARCHAR
    };
    EXPECT_FALSE(db->createTable("users", columns));
    
    // Try to drop non-existent table
    db->open("test_db");
    EXPECT_FALSE(db->dropTable("non_existent"));
    
    // Try to insert into non-existent table
    std::vector<std::string> values = {"1", "John"};
    EXPECT_FALSE(db->insert("non_existent", values));
    
    // Try to select from non-existent table
    EXPECT_FALSE(db->select("non_existent", {"id"}, "", 
        [](const std::vector<std::string>&){}));
    
    // Try to delete from non-existent table
    EXPECT_FALSE(db->delete_("non_existent", "id = 1"));
    
    // Try to describe non-existent table
    EXPECT_FALSE(db->describe("non_existent"));
} 