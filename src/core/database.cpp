#include "core/database.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <functional>

namespace preql {
namespace core {

class Database::Impl {
public:
    Impl() : is_open_(false) {}
    
    bool create(const std::string& name, size_t num_pages) {
        if (is_open_) {
            return false;
        }
        
        std::string db_path = DBPATH + name;
        std::ofstream db_file(db_path, std::ios::binary);
        if (!db_file) {
            return false;
        }
        
        // Write header page
        dp_page header;
        header.page_num = HEADER_PAGE;
        header.next_free = EMPTY;
        header.num_records = 0;
        db_file.write(reinterpret_cast<char*>(&header), sizeof(dp_page));
        
        // Initialize remaining pages
        for (size_t i = 1; i < num_pages; ++i) {
            dp_page page;
            page.page_num = i + 1;
            page.next_free = EMPTY;
            page.num_records = 0;
            db_file.write(reinterpret_cast<char*>(&page), sizeof(dp_page));
        }
        
        // Create system table
        std::string sys_table_path = DBPATH + name + "_sys";
        std::ofstream sys_table(sys_table_path, std::ios::binary);
        if (!sys_table) {
            std::filesystem::remove(db_path);
            return false;
        }
        
        db_name_ = name;
        is_open_ = true;
        return true;
    }
    
    bool drop(const std::string& name) {
        if (is_open_ && db_name_ == name) {
            close();
        }
        
        std::string db_path = DBPATH + name;
        std::string sys_table_path = DBPATH + name + "_sys";
        
        bool success = true;
        success &= std::filesystem::remove(db_path);
        success &= std::filesystem::remove(sys_table_path);
        
        // Remove all table files
        for (const auto& entry : std::filesystem::directory_iterator(DBPATH)) {
            if (entry.path().string().find(name + "_") == 0) {
                success &= std::filesystem::remove(entry.path());
            }
        }
        
        return success;
    }
    
    bool open(const std::string& name) {
        if (is_open_) {
            return false;
        }
        
        std::string db_path = DBPATH + name;
        if (!std::filesystem::exists(db_path)) {
            return false;
        }
        
        db_name_ = name;
        is_open_ = true;
        return true;
    }
    
    bool close() {
        if (!is_open_) {
            return false;
        }
        
        is_open_ = false;
        db_name_.clear();
        return true;
    }
    
    bool isOpen() const {
        return is_open_;
    }
    
    bool createTable(const std::string& name, 
                    const std::vector<std::pair<std::string, int>>& columns) {
        if (!is_open_) {
            return false;
        }
        
        // Check if table already exists
        if (tableExists(name)) {
            return false;
        }
        
        // Create system table entry
        mega_struct table_info;
        strncpy(table_info.table_name, name.c_str(), MAX_TABLE_NAME);
        table_info.num_columns = columns.size();
        
        // Write table metadata to system table
        std::string sys_table_path = DBPATH + db_name_ + "_sys";
        std::ofstream sys_table(sys_table_path, std::ios::binary | std::ios::app);
        if (!sys_table) {
            return false;
        }
        
        sys_table.write(reinterpret_cast<char*>(&table_info), sizeof(mega_struct));
        
        // Create table file
        std::string table_path = DBPATH + db_name_ + "_" + name;
        std::ofstream table_file(table_path, std::ios::binary);
        if (!table_file) {
            return false;
        }
        
        // Write column definitions
        for (const auto& col : columns) {
            column_def col_def;
            strncpy(col_def.name, col.first.c_str(), MAX_COL_NAME);
            col_def.type = col.second;
            table_file.write(reinterpret_cast<char*>(&col_def), sizeof(column_def));
        }
        
        return true;
    }
    
    bool dropTable(const std::string& name) {
        if (!is_open_) {
            return false;
        }
        
        // Remove from system table
        std::string sys_table_path = DBPATH + db_name_ + "_sys";
        std::ifstream sys_table(sys_table_path, std::ios::binary);
        if (!sys_table) {
            return false;
        }
        
        std::vector<mega_struct> tables;
        mega_struct table_info;
        while (sys_table.read(reinterpret_cast<char*>(&table_info), sizeof(mega_struct))) {
            if (std::string(table_info.table_name) != name) {
                tables.push_back(table_info);
            }
        }
        
        // Rewrite system table without the dropped table
        std::ofstream sys_table_out(sys_table_path, std::ios::binary | std::ios::trunc);
        if (!sys_table_out) {
            return false;
        }
        
        for (const auto& table : tables) {
            sys_table_out.write(reinterpret_cast<char*>(const_cast<mega_struct*>(&table)), 
                              sizeof(mega_struct));
        }
        
        // Delete table file
        std::string table_path = DBPATH + db_name_ + "_" + name;
        return std::filesystem::remove(table_path);
    }
    
    bool insert(const std::string& table_name, 
                const std::vector<std::string>& values) {
        if (!is_open_) {
            return false;
        }
        
        // Get table metadata
        std::vector<column_def> columns = getTableColumns(table_name);
        if (columns.empty() || columns.size() != values.size()) {
            return false;
        }
        
        // Validate and convert values
        std::vector<record> records;
        for (size_t i = 0; i < values.size(); ++i) {
            record rec;
            if (!convertValue(values[i], columns[i].type, rec)) {
                return false;
            }
            records.push_back(rec);
        }
        
        // Write records to table
        std::string table_path = DBPATH + db_name_ + "_" + table_name;
        std::ofstream table_file(table_path, std::ios::binary | std::ios::app);
        if (!table_file) {
            return false;
        }
        
        for (const auto& rec : records) {
            table_file.write(reinterpret_cast<const char*>(&rec), sizeof(record));
        }
        
        return true;
    }
    
    bool select(const std::string& table_name,
                const std::vector<std::string>& columns,
                const std::string& condition,
                std::function<void(const std::vector<std::string>&)> row_callback) {
        if (!is_open_) {
            return false;
        }
        
        // Get table metadata
        std::vector<column_def> table_columns = getTableColumns(table_name);
        if (table_columns.empty()) {
            return false;
        }
        
        // Validate columns
        std::vector<size_t> selected_indices;
        if (columns.size() == 1 && columns[0] == "*") {
            for (size_t i = 0; i < table_columns.size(); ++i) {
                selected_indices.push_back(i);
            }
        } else {
            for (const auto& col_name : columns) {
                auto it = std::find_if(table_columns.begin(), table_columns.end(),
                    [&](const column_def& col) {
                        return std::string(col.name) == col_name;
                    });
                if (it == table_columns.end()) {
                    return false;
                }
                selected_indices.push_back(std::distance(table_columns.begin(), it));
            }
        }
        
        // Read and filter records
        std::string table_path = DBPATH + db_name_ + "_" + table_name;
        std::ifstream table_file(table_path, std::ios::binary);
        if (!table_file) {
            return false;
        }
        
        record rec;
        while (table_file.read(reinterpret_cast<char*>(&rec), sizeof(record))) {
            if (evaluateCondition(rec, table_columns, condition)) {
                std::vector<std::string> row;
                for (size_t idx : selected_indices) {
                    row.push_back(convertToString(rec, table_columns[idx].type));
                }
                if (row_callback) {
                    row_callback(row);
                }
            }
        }
        
        return true;
    }
    
    bool delete_(const std::string& table_name, const std::string& condition) {
        if (!is_open_) {
            return false;
        }
        
        // Get table metadata
        std::vector<column_def> columns = getTableColumns(table_name);
        if (columns.empty()) {
            return false;
        }
        
        // Read all records
        std::string table_path = DBPATH + db_name_ + "_" + table_name;
        std::ifstream table_file(table_path, std::ios::binary);
        if (!table_file) {
            return false;
        }
        
        std::vector<record> records;
        record rec;
        while (table_file.read(reinterpret_cast<char*>(&rec), sizeof(record))) {
            if (!evaluateCondition(rec, columns, condition)) {
                records.push_back(rec);
            }
        }
        
        // Rewrite table without deleted records
        std::ofstream table_file_out(table_path, std::ios::binary | std::ios::trunc);
        if (!table_file_out) {
            return false;
        }
        
        for (const auto& record : records) {
            table_file_out.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        
        return true;
    }
    
    bool describe(const std::string& table_name) {
        if (!is_open_) {
            return false;
        }
        
        std::vector<column_def> columns = getTableColumns(table_name);
        if (columns.empty()) {
            return false;
        }
        
        // Print table information
        std::cout << "Table: " << table_name << "\n";
        std::cout << "Columns:\n";
        std::cout << std::setw(20) << "Name" << std::setw(10) << "Type" << "\n";
        std::cout << std::string(30, '-') << "\n";
        
        for (const auto& col : columns) {
            std::cout << std::setw(20) << col.name << std::setw(10);
            switch (col.type) {
                case INT:
                    std::cout << "INT";
                    break;
                case FLOAT:
                    std::cout << "FLOAT";
                    break;
                case VARCHAR:
                    std::cout << "VARCHAR";
                    break;
                case CHAR:
                    std::cout << "CHAR";
                    break;
                default:
                    std::cout << "UNKNOWN";
            }
            std::cout << "\n";
        }
        
        return true;
    }
    
    std::vector<std::string> listTables() const {
        if (!is_open_) {
            return {};
        }
        
        std::vector<std::string> tables;
        std::string sys_table_path = DBPATH + db_name_ + "_sys";
        std::ifstream sys_table(sys_table_path, std::ios::binary);
        if (!sys_table) {
            return tables;
        }
        
        mega_struct table_info;
        while (sys_table.read(reinterpret_cast<char*>(&table_info), sizeof(mega_struct))) {
            tables.push_back(table_info.table_name);
        }
        
        return tables;
    }

private:
    bool is_open_;
    std::string db_name_;
    
    bool tableExists(const std::string& name) const {
        auto tables = listTables();
        return std::find(tables.begin(), tables.end(), name) != tables.end();
    }
    
    std::vector<column_def> getTableColumns(const std::string& table_name) {
        std::vector<column_def> columns;
        std::string table_path = DBPATH + db_name_ + "_" + table_name;
        std::ifstream table_file(table_path, std::ios::binary);
        if (!table_file) {
            return columns;
        }
        
        column_def col;
        while (table_file.read(reinterpret_cast<char*>(&col), sizeof(column_def))) {
            columns.push_back(col);
        }
        
        return columns;
    }
    
    bool convertValue(const std::string& value, int type, record& rec) {
        try {
            switch (type) {
                case INT:
                    rec.int_val = std::stoi(value);
                    break;
                case FLOAT:
                    rec.float_val = std::stof(value);
                    break;
                case VARCHAR:
                case CHAR:
                    strncpy(rec.str_val, value.c_str(), MAX_STR_LEN);
                    break;
                default:
                    return false;
            }
            return true;
        } catch (...) {
            return false;
        }
    }
    
    std::string convertToString(const record& rec, int type) {
        switch (type) {
            case INT:
                return std::to_string(rec.int_val);
            case FLOAT:
                return std::to_string(rec.float_val);
            case VARCHAR:
            case CHAR:
                return rec.str_val;
            default:
                return "";
        }
    }
    
    bool evaluateCondition(const record& rec,
                          const std::vector<column_def>& columns,
                          const std::string& condition) {
        if (condition.empty()) {
            return true;
        }
        
        std::istringstream iss(condition);
        std::string col_name, op, value;
        
        // Parse condition: column operator value
        if (!(iss >> col_name >> op >> value)) {
            return false;
        }
        
        // Find column index
        auto it = std::find_if(columns.begin(), columns.end(),
            [&](const column_def& col) {
                return std::string(col.name) == col_name;
            });
        
        if (it == columns.end()) {
            return false;
        }
        
        size_t col_idx = std::distance(columns.begin(), it);
        
        // Get column value
        std::string col_value = convertToString(rec, it->type);
        
        // Evaluate condition
        if (op == "=") {
            return col_value == value;
        } else if (op == "!=") {
            return col_value != value;
        } else if (op == "<") {
            if (it->type == INT) {
                return std::stoi(col_value) < std::stoi(value);
            } else if (it->type == FLOAT) {
                return std::stof(col_value) < std::stof(value);
            } else {
                return col_value < value;
            }
        } else if (op == ">") {
            if (it->type == INT) {
                return std::stoi(col_value) > std::stoi(value);
            } else if (it->type == FLOAT) {
                return std::stof(col_value) > std::stof(value);
            } else {
                return col_value > value;
            }
        } else if (op == "<=") {
            if (it->type == INT) {
                return std::stoi(col_value) <= std::stoi(value);
            } else if (it->type == FLOAT) {
                return std::stof(col_value) <= std::stof(value);
            } else {
                return col_value <= value;
            }
        } else if (op == ">=") {
            if (it->type == INT) {
                return std::stoi(col_value) >= std::stoi(value);
            } else if (it->type == FLOAT) {
                return std::stof(col_value) >= std::stof(value);
            } else {
                return col_value >= value;
            }
        } else if (op == "LIKE") {
            // Simple LIKE implementation (exact match for now)
            return col_value == value;
        } else {
            return false;
        }
    }
};

// Database class implementation
Database::Database() : pimpl_(std::make_unique<Impl>()) {}
Database::~Database() = default;

bool Database::create(const std::string& name, size_t num_pages) {
    return pimpl_->create(name, num_pages);
}

bool Database::drop(const std::string& name) {
    return pimpl_->drop(name);
}

bool Database::open(const std::string& name) {
    return pimpl_->open(name);
}

bool Database::close() {
    return pimpl_->close();
}

bool Database::isOpen() const {
    return pimpl_->isOpen();
}

bool Database::createTable(const std::string& name, 
                          const std::vector<std::pair<std::string, int>>& columns) {
    return pimpl_->createTable(name, columns);
}

bool Database::dropTable(const std::string& name) {
    return pimpl_->dropTable(name);
}

bool Database::insert(const std::string& table_name, const std::vector<std::string>& values) {
    return pimpl_->insert(table_name, values);
}

bool Database::select(const std::string& table_name,
                     const std::vector<std::string>& columns,
                     const std::string& condition,
                     std::function<void(const std::vector<std::string>&)> row_callback) {
    return pimpl_->select(table_name, columns, condition, row_callback);
}

bool Database::delete_(const std::string& table_name, const std::string& condition) {
    return pimpl_->delete_(table_name, condition);
}

bool Database::describe(const std::string& table_name) {
    return pimpl_->describe(table_name);
}

std::vector<std::string> Database::listTables() const {
    return pimpl_->listTables();
}

} // namespace core
} // namespace preql 