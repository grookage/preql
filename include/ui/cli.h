#pragma once

#include <string>
#include <memory>
#include <functional>

namespace preql {
namespace ui {

class CLI {
public:
    CLI();
    ~CLI();

    void run();
    void stop();

    // Command handlers
    using CommandHandler = std::function<void(const std::string&)>;
    void registerCommand(const std::string& cmd, CommandHandler handler);
    void unregisterCommand(const std::string& cmd);

    // Output formatting
    void printTable(const std::vector<std::string>& headers, 
                   const std::vector<std::vector<std::string>>& rows);
    void printError(const std::string& message);
    void printSuccess(const std::string& message);
    void printInfo(const std::string& message);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ui
} // namespace preql 