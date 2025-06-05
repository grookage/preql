#include "sql/parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace preql {
namespace sql {

class Parser::Impl {
public:
    SQLStatement parse(const std::string& query) {
        std::istringstream iss(query);
        std::string token;
        
        if (!(iss >> token)) {
            throw std::runtime_error("Empty query");
        }
        
        std::transform(token.begin(), token.end(), token.begin(), ::toupper);
        
        if (token == "CREATE") {
            return parseCreateTable(iss);
        } else if (token == "INSERT") {
            return parseInsert(iss);
        } else if (token == "SELECT") {
            return parseSelect(iss);
        } else if (token == "DELETE") {
            return parseDelete(iss);
        } else {
            throw std::runtime_error("Unknown command: " + token);
        }
    }
    
    bool validate(const SQLStatement& statement) {
        return std::visit([this](const auto& stmt) {
            return validateStatement(stmt);
        }, statement);
    }

private:
    SQLStatement parseCreateTable(std::istringstream& iss) {
        std::string token;
        CreateTableStatement stmt;
        
        // Parse TABLE keyword
        if (!(iss >> token) || std::toupper(token[0]) != 'T') {
            throw std::runtime_error("Expected TABLE keyword");
        }
        
        // Parse table name
        if (!(iss >> stmt.table_name)) {
            throw std::runtime_error("Expected table name");
        }
        
        // Parse opening parenthesis
        if (!(iss >> token) || token != "(") {
            throw std::runtime_error("Expected opening parenthesis");
        }
        
        // Parse column definitions
        while (true) {
            ColumnDefinition col;
            
            // Parse column name
            if (!(iss >> col.name)) {
                throw std::runtime_error("Expected column name");
            }
            
            // Parse column type
            if (!(iss >> token)) {
                throw std::runtime_error("Expected column type");
            }
            
            std::transform(token.begin(), token.end(), token.begin(), ::toupper);
            if (token == "INT") {
                col.type = INT;
            } else if (token == "VARCHAR") {
                col.type = VARCHAR;
            } else if (token == "CHAR") {
                col.type = CHAR;
            } else if (token == "FLOAT") {
                col.type = FLOAT;
            } else {
                throw std::runtime_error("Unknown column type: " + token);
            }
            
            // Parse constraints
            col.is_primary_key = false;
            col.is_nullable = true;
            
            while (iss >> token) {
                std::transform(token.begin(), token.end(), token.begin(), ::toupper);
                if (token == "PRIMARY" && iss >> token && token == "KEY") {
                    col.is_primary_key = true;
                } else if (token == "NOT" && iss >> token && token == "NULL") {
                    col.is_nullable = false;
                } else if (token == ",") {
                    break;
                } else if (token == ")") {
                    stmt.columns.push_back(col);
                    return stmt;
                } else {
                    throw std::runtime_error("Unexpected token: " + token);
                }
            }
            
            stmt.columns.push_back(col);
        }
    }
    
    SQLStatement parseInsert(std::istringstream& iss) {
        std::string token;
        InsertStatement stmt;
        
        // Parse INTO keyword
        if (!(iss >> token) || std::toupper(token[0]) != 'I') {
            throw std::runtime_error("Expected INTO keyword");
        }
        
        // Parse table name
        if (!(iss >> stmt.table_name)) {
            throw std::runtime_error("Expected table name");
        }
        
        // Parse VALUES keyword
        if (!(iss >> token) || std::toupper(token[0]) != 'V') {
            throw std::runtime_error("Expected VALUES keyword");
        }
        
        // Parse opening parenthesis
        if (!(iss >> token) || token != "(") {
            throw std::runtime_error("Expected opening parenthesis");
        }
        
        // Parse values
        while (true) {
            std::string value;
            if (!(iss >> value)) {
                throw std::runtime_error("Expected value");
            }
            
            stmt.values.push_back(value);
            
            if (!(iss >> token)) {
                throw std::runtime_error("Expected closing parenthesis or comma");
            }
            
            if (token == ")") {
                break;
            } else if (token != ",") {
                throw std::runtime_error("Expected comma or closing parenthesis");
            }
        }
        
        return stmt;
    }
    
    SQLStatement parseSelect(std::istringstream& iss) {
        std::string token;
        SelectStatement stmt;
        
        // Parse column list
        while (true) {
            if (!(iss >> token)) {
                throw std::runtime_error("Expected column name or *");
            }
            
            if (token == "*") {
                stmt.columns.clear();
                stmt.columns.push_back("*");
                break;
            }
            
            stmt.columns.push_back(token);
            
            if (!(iss >> token)) {
                throw std::runtime_error("Expected comma or FROM keyword");
            }
            
            if (token == "FROM") {
                break;
            } else if (token != ",") {
                throw std::runtime_error("Expected comma or FROM keyword");
            }
        }
        
        // Parse table name
        if (!(iss >> stmt.table_name)) {
            throw std::runtime_error("Expected table name");
        }
        
        // Parse WHERE clause if present
        if (iss >> token && std::toupper(token[0]) == 'W') {
            std::string condition;
            while (iss >> token) {
                condition += token + " ";
            }
            stmt.condition = condition;
        }
        
        return stmt;
    }
    
    SQLStatement parseDelete(std::istringstream& iss) {
        std::string token;
        DeleteStatement stmt;
        
        // Parse FROM keyword
        if (!(iss >> token) || std::toupper(token[0]) != 'F') {
            throw std::runtime_error("Expected FROM keyword");
        }
        
        // Parse table name
        if (!(iss >> stmt.table_name)) {
            throw std::runtime_error("Expected table name");
        }
        
        // Parse WHERE clause if present
        if (iss >> token && std::toupper(token[0]) == 'W') {
            std::string condition;
            while (iss >> token) {
                condition += token + " ";
            }
            stmt.condition = condition;
        }
        
        return stmt;
    }
    
    bool validateStatement(const CreateTableStatement& stmt) {
        if (stmt.table_name.empty()) {
            return false;
        }
        
        if (stmt.columns.empty()) {
            return false;
        }
        
        for (const auto& col : stmt.columns) {
            if (col.name.empty()) {
                return false;
            }
            if (col.type < INT || col.type > FLOAT) {
                return false;
            }
        }
        
        return true;
    }
    
    bool validateStatement(const InsertStatement& stmt) {
        if (stmt.table_name.empty()) {
            return false;
        }
        
        if (stmt.values.empty()) {
            return false;
        }
        
        return true;
    }
    
    bool validateStatement(const SelectStatement& stmt) {
        if (stmt.table_name.empty()) {
            return false;
        }
        
        if (stmt.columns.empty()) {
            return false;
        }
        
        return true;
    }
    
    bool validateStatement(const DeleteStatement& stmt) {
        if (stmt.table_name.empty()) {
            return false;
        }
        
        return true;
    }
};

// Parser class implementation
Parser::Parser() : pimpl_(std::make_unique<Impl>()) {}
Parser::~Parser() = default;

SQLStatement Parser::parse(const std::string& query) {
    return pimpl_->parse(query);
}

bool Parser::validate(const SQLStatement& statement) {
    return pimpl_->validate(statement);
}

} // namespace sql
} // namespace preql 