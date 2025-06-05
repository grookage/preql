#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace preql {
namespace sql {

struct ColumnDefinition {
    std::string name;
    int type;
    bool is_primary_key;
    bool is_nullable;
};

struct CreateTableStatement {
    std::string table_name;
    std::vector<ColumnDefinition> columns;
};

struct InsertStatement {
    std::string table_name;
    std::vector<std::string> values;
};

struct SelectStatement {
    std::string table_name;
    std::vector<std::string> columns;
    std::string condition;
};

struct DeleteStatement {
    std::string table_name;
    std::string condition;
};

using SQLStatement = std::variant<
    CreateTableStatement,
    InsertStatement,
    SelectStatement,
    DeleteStatement
>;

class Parser {
public:
    Parser();
    ~Parser();

    SQLStatement parse(const std::string& query);
    bool validate(const SQLStatement& statement);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace sql
} // namespace preql 