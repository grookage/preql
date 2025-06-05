#include <gtest/gtest.h>
#include "ui/cli.h"
#include <sstream>
#include <iostream>

using namespace preql;

class CLITest : public ::testing::Test {
protected:
    void SetUp() override {
        cli = std::make_unique<ui::CLI>();
    }

    void TearDown() override {
        cli.reset();
    }

    std::unique_ptr<ui::CLI> cli;
};

TEST_F(CLITest, RegisterAndUnregisterCommand) {
    bool command_executed = false;
    
    // Register a test command
    cli->registerCommand("TEST", [&command_executed](const std::string& args) {
        command_executed = true;
    });
    
    // Execute the command
    cli->executeCommand("TEST");
    EXPECT_TRUE(command_executed);
    
    // Unregister the command
    cli->unregisterCommand("TEST");
    command_executed = false;
    
    // Try to execute the unregistered command
    cli->executeCommand("TEST");
    EXPECT_FALSE(command_executed);
}

TEST_F(CLITest, PrintTable) {
    std::vector<std::string> headers = {"ID", "Name", "Age"};
    std::vector<std::vector<std::string>> rows = {
        {"1", "John", "25"},
        {"2", "Jane", "30"}
    };
    
    // Redirect cout to a stringstream
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Print the table
    cli->printTable(headers, rows);
    
    // Restore cout
    std::cout.rdbuf(old);
    
    // Verify output
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("ID") != std::string::npos);
    EXPECT_TRUE(output.find("Name") != std::string::npos);
    EXPECT_TRUE(output.find("Age") != std::string::npos);
    EXPECT_TRUE(output.find("John") != std::string::npos);
    EXPECT_TRUE(output.find("Jane") != std::string::npos);
}

TEST_F(CLITest, PrintError) {
    // Redirect cout to a stringstream
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Print error message
    cli->printError("Test error message");
    
    // Restore cout
    std::cout.rdbuf(old);
    
    // Verify output
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Error") != std::string::npos);
    EXPECT_TRUE(output.find("Test error message") != std::string::npos);
}

TEST_F(CLITest, PrintSuccess) {
    // Redirect cout to a stringstream
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Print success message
    cli->printSuccess("Test success message");
    
    // Restore cout
    std::cout.rdbuf(old);
    
    // Verify output
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Success") != std::string::npos);
    EXPECT_TRUE(output.find("Test success message") != std::string::npos);
}

TEST_F(CLITest, PrintInfo) {
    // Redirect cout to a stringstream
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Print info message
    cli->printInfo("Test info message");
    
    // Restore cout
    std::cout.rdbuf(old);
    
    // Verify output
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Info") != std::string::npos);
    EXPECT_TRUE(output.find("Test info message") != std::string::npos);
}

TEST_F(CLITest, ShowHelp) {
    // Register some test commands
    cli->registerCommand("TEST1", [](const std::string&) {});
    cli->registerCommand("TEST2", [](const std::string&) {});
    
    // Redirect cout to a stringstream
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Show help
    cli->showHelp();
    
    // Restore cout
    std::cout.rdbuf(old);
    
    // Verify output
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Available commands") != std::string::npos);
    EXPECT_TRUE(output.find("TEST1") != std::string::npos);
    EXPECT_TRUE(output.find("TEST2") != std::string::npos);
}

TEST_F(CLITest, CommandArguments) {
    std::string received_args;
    
    // Register a command that captures arguments
    cli->registerCommand("TEST", [&received_args](const std::string& args) {
        received_args = args;
    });
    
    // Execute command with arguments
    cli->executeCommand("TEST arg1 arg2 arg3");
    EXPECT_EQ(received_args, "arg1 arg2 arg3");
}

TEST_F(CLITest, StopRunning) {
    // Start CLI in a separate thread
    std::thread cli_thread([this]() {
        cli->run();
    });
    
    // Wait a bit to ensure CLI is running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop the CLI
    cli->stop();
    
    // Wait for thread to finish
    cli_thread.join();
    
    // Verify CLI is stopped
    EXPECT_FALSE(cli->isRunning());
} 