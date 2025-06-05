cmake_minimum_required(VERSION 3.10)
project(preql_tests)

# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executables
add_executable(database_test database_test.cpp)
add_executable(buffer_test buffer_test.cpp)
add_executable(parser_test parser_test.cpp)
add_executable(cli_test cli_test.cpp)

# Link against GTest and our library
target_link_libraries(database_test ${GTEST_LIBRARIES} pthread preql)
target_link_libraries(buffer_test ${GTEST_LIBRARIES} pthread preql)
target_link_libraries(parser_test ${GTEST_LIBRARIES} pthread preql)
target_link_libraries(cli_test ${GTEST_LIBRARIES} pthread preql)

# Add test
add_test(NAME database_test COMMAND database_test)
add_test(NAME buffer_test COMMAND buffer_test)
add_test(NAME parser_test COMMAND parser_test)
add_test(NAME cli_test COMMAND cli_test) 