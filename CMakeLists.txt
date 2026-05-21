# CMake version on the machine < Requierd -> Error
cmake_minimum_required(VERSION 3.20)

# project name: CodeGhost. Used languages C++
project(CodeGhost LANGUAGES CXX)

# fixed standard for compiling (C++20)
set(CMAKE_CXX_STANDARD 20)

# Choosed standard is required and if compiler doesnt support it -> Error
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specific extensions are disabled. Requires clean mode
set(CMAKE_CXX_EXTENSIONS OFF)

# Module 'FetchContent' enable which allows you to load external dependencies
include(FetchContent)

# Finds system threads library. If not found -> Error
find_package(Threads REQUIRED)

# Announces external resource (googletest) with URL archive. Just announce not download
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Downloads and make available announced googletest
FetchContent_MakeAvailable(googletest)

# Makes library  'codeghost_core' with specified files (object or static)
add_library(codeghost_core
    src/cli/cli.cpp
    src/core/daemon.cpp
    src/core/eventprocessor.cpp
    src/core/hasher.cpp
    src/core/logger.cpp
    src/core/watcher.cpp
    src/utils/utils.cpp
)

# Adds path include directory for 'codeghost_core'. Because of PUBLIC this directory will be used by the library itself
target_include_directories(codeghost_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link 'codeghost_core' with Threads::Threads and xxhash libraries
target_link_libraries(codeghost_core
    PUBLIC
        Threads::Threads
        xxhash
)

# Create executable file codeghost from main.cpp
add_executable(codeghost
    src/main.cpp
)

target_link_libraries(codeghost
    PRIVATE
        codeghost_core
)

# Enable testing CTest/GTest support. Include ctest() command and tests.
enable_testing()

# Create executable test binary file 'codeghost_tests'
add_executable(codeghost_tests
    tests/integration/filesystem_monitoring_test.cpp
    tests/integration/watcher_delete_test.cpp
    tests/integration/watcher_modify_test.cpp
    tests/integration/watcher_recursive_test.cpp
    tests/integration/watcher_ignore_paths_test.cpp
)

# Links tets binary files with 'codeghost_core' and GTest::gtest_main. PRIVATE flag means that dependencies are not allowd to export further
target_link_libraries(codeghost_tests
    PRIVATE
        codeghost_core
        GTest::gtest_main
)

# Connect GoogleTest module
include(GoogleTest)

# Register tests automatically found in codeghost_tests
gtest_discover_tests(codeghost_tests)
