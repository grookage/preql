cmake_minimum_required(VERSION 3.10)
project(preql VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/database.cpp
    src/buffer/buffer_manager.cpp
    src/sql/parser.cpp
    src/ui/cli.cpp
)

# Create executable
add_executable(preql ${SOURCES})

# Link libraries
target_link_libraries(preql PRIVATE
    pthread
)

# Install
install(TARGETS preql DESTINATION bin) 