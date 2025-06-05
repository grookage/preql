#include "core/database.h"
#include "buffer/buffer_manager.h"
#include "sql/parser.h"
#include "ui/cli.h"
#include <iostream>
#include <memory>

using namespace preql;

int main(int argc, char* argv[]) {
    try {
        // Initialize components
        auto db = std::make_unique<core::Database>();
        auto buffer = std::make_unique<buffer::BufferManager>();
        auto parser = std::make_unique<sql::Parser>();
        auto cli = std::make_unique<ui::CLI>();

        // Register command handlers
        cli->registerCommand("CREATE", [&](const std::string& args) {
            auto stmt = parser->parse(args);
            if (auto create_stmt = std::get_if<sql::CreateTableStatement>(&stmt)) {
                std::vector<std::pair<std::string, int>> columns;
                for (const auto& col : create_stmt->columns) {
                    columns.emplace_back(col.name, col.type);
                }
                if (db->createTable(create_stmt->table_name, columns)) {
                    cli->printSuccess("Table created successfully");
                } else {
                    cli->printError("Failed to create table");
                }
            }
        });

        cli->registerCommand("INSERT", [&](const std::string& args) {
            auto stmt = parser->parse(args);
            if (auto insert_stmt = std::get_if<sql::InsertStatement>(&stmt)) {
                if (db->insert(insert_stmt->table_name, insert_stmt->values)) {
                    cli->printSuccess("Data inserted successfully");
                } else {
                    cli->printError("Failed to insert data");
                }
            }
        });

        cli->registerCommand("SELECT", [&](const std::string& args) {
            auto stmt = parser->parse(args);
            if (auto select_stmt = std::get_if<sql::SelectStatement>(&stmt)) {
                std::vector<std::vector<std::string>> results;
                std::vector<std::string> headers;
                
                // Get column names for headers
                if (select_stmt->columns.size() == 1 && select_stmt->columns[0] == "*") {
                    // TODO: Get all column names from table metadata
                    headers = select_stmt->columns;
                } else {
                    headers = select_stmt->columns;
                }
                
                // Execute select with callback
                if (db->select(select_stmt->table_name,
                             select_stmt->columns,
                             select_stmt->condition,
                             [&](const std::vector<std::string>& row) {
                                 results.push_back(row);
                             })) {
                    cli->printTable(headers, results);
                } else {
                    cli->printError("Failed to execute query");
                }
            }
        });

        cli->registerCommand("DELETE", [&](const std::string& args) {
            auto stmt = parser->parse(args);
            if (auto delete_stmt = std::get_if<sql::DeleteStatement>(&stmt)) {
                if (db->delete_(delete_stmt->table_name, delete_stmt->condition)) {
                    cli->printSuccess("Records deleted successfully");
                } else {
                    cli->printError("Failed to delete records");
                }
            }
        });

        cli->registerCommand("DESCRIBE", [&](const std::string& args) {
            auto stmt = parser->parse(args);
            if (auto describe_stmt = std::get_if<sql::DescribeStatement>(&stmt)) {
                if (db->describe(describe_stmt->table_name)) {
                    cli->printSuccess("Table described successfully");
                } else {
                    cli->printError("Failed to describe table");
                }
            }
        });

        // Start the CLI
        cli->run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 