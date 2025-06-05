#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace preql {
namespace core {

class Database {
public:
    Database();
    ~Database();

    // Database operations
    bool create(const std::string& name, size_t num_pages);
    bool drop(const std::string& name);
    bool open(const std::string& name);
    bool close();
    bool isOpen() const;

    // Table operations
    bool createTable(const std::string& name, const std::vector<std::pair<std::string, int>>& columns);
    bool dropTable(const std::string& name);
    bool insert(const std::string& table_name, const std::vector<std::string>& values);
    bool select(const std::string& table_name,
               const std::vector<std::string>& columns,
               const std::string& condition,
               std::function<void(const std::vector<std::string>&)> row_callback = nullptr);
    bool delete_(const std::string& table_name, const std::string& condition);
    bool describe(const std::string& table_name);
    std::vector<std::string> listTables() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace preql 