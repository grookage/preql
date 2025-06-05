#include "ui/cli.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

namespace preql {
namespace ui {

class CLI::Impl {
public:
    Impl() : running_(false) {}
    
    void run() {
        running_ = true;
        std::string input;
        
        std::cout << "Welcome to PreQL Database Management System\n"
                  << "Type 'help' for available commands\n\n";
        
        while (running_) {
            std::cout << "preql> ";
            std::getline(std::cin, input);
            
            if (input.empty()) {
                continue;
            }
            
            if (input == "exit" || input == "quit") {
                running_ = false;
                continue;
            }
            
            if (input == "help") {
                showHelp();
                continue;
            }
            
            // Parse command
            std::istringstream iss(input);
            std::string cmd;
            iss >> cmd;
            
            // Get command arguments
            std::string args;
            std::getline(iss, args);
            if (!args.empty() && args[0] == ' ') {
                args = args.substr(1);
            }
            
            // Execute command
            auto it = commands_.find(cmd);
            if (it != commands_.end()) {
                try {
                    it->second(args);
                } catch (const std::exception& e) {
                    printError(e.what());
                }
            } else {
                printError("Unknown command: " + cmd);
            }
        }
    }
    
    void stop() {
        running_ = false;
    }
    
    void registerCommand(const std::string& cmd, CommandHandler handler) {
        commands_[cmd] = handler;
    }
    
    void unregisterCommand(const std::string& cmd) {
        commands_.erase(cmd);
    }
    
    void printTable(const std::vector<std::string>& headers,
                   const std::vector<std::vector<std::string>>& rows) {
        if (headers.empty() || rows.empty()) {
            return;
        }
        
        // Calculate column widths
        std::vector<size_t> col_widths(headers.size());
        for (size_t i = 0; i < headers.size(); ++i) {
            col_widths[i] = headers[i].length();
        }
        
        for (const auto& row : rows) {
            for (size_t i = 0; i < row.size(); ++i) {
                col_widths[i] = std::max(col_widths[i], row[i].length());
            }
        }
        
        // Print header
        printTableRow(headers, col_widths);
        
        // Print separator
        for (size_t width : col_widths) {
            std::cout << std::string(width + 2, '-');
        }
        std::cout << '\n';
        
        // Print rows
        for (const auto& row : rows) {
            printTableRow(row, col_widths);
        }
        std::cout << '\n';
    }
    
    void printError(const std::string& message) {
        std::cerr << "\033[1;31mError: " << message << "\033[0m\n";
    }
    
    void printSuccess(const std::string& message) {
        std::cout << "\033[1;32m" << message << "\033[0m\n";
    }
    
    void printInfo(const std::string& message) {
        std::cout << "\033[1;34m" << message << "\033[0m\n";
    }

private:
    bool running_;
    std::map<std::string, CommandHandler> commands_;
    
    void showHelp() {
        std::cout << "Available commands:\n";
        for (const auto& cmd : commands_) {
            std::cout << "  " << cmd.first << "\n";
        }
        std::cout << "  help    - Show this help message\n"
                  << "  exit    - Exit the program\n"
                  << "  quit    - Exit the program\n\n";
    }
    
    void printTableRow(const std::vector<std::string>& row,
                      const std::vector<size_t>& col_widths) {
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << std::setw(col_widths[i]) << row[i] << "  ";
        }
        std::cout << '\n';
    }
};

// CLI class implementation
CLI::CLI() : pimpl_(std::make_unique<Impl>()) {}
CLI::~CLI() = default;

void CLI::run() {
    pimpl_->run();
}

void CLI::stop() {
    pimpl_->stop();
}

void CLI::registerCommand(const std::string& cmd, CommandHandler handler) {
    pimpl_->registerCommand(cmd, handler);
}

void CLI::unregisterCommand(const std::string& cmd) {
    pimpl_->unregisterCommand(cmd);
}

void CLI::printTable(const std::vector<std::string>& headers,
                    const std::vector<std::vector<std::string>>& rows) {
    pimpl_->printTable(headers, rows);
}

void CLI::printError(const std::string& message) {
    pimpl_->printError(message);
}

void CLI::printSuccess(const std::string& message) {
    pimpl_->printSuccess(message);
}

void CLI::printInfo(const std::string& message) {
    pimpl_->printInfo(message);
}

} // namespace ui
} // namespace preql 