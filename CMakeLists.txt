cmake_minimum_required(VERSION 3.25) # minimum version of CMake to build the project
project(CodeGhost LANGUAGES CXX)     # announces the project and specifies the language to be used
set(CMAKE_CXX_STANDARD 23)           # defines the C++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # requires a strictly specified standard
set(CMAKE_CXX_EXTENSIONS OFF)        # disables compiler extensions
if (MSVC)       
    add_compile_options(/W4 /WX)     # for Visual Studio: W4 - high level of warnings WX - warnings to errors
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror) # GCC/Clang: -Wall - basic warnings -Wextra - additional -Wpedantic - strict standard checking -Werror - warnings in errors
endif()

add_library(codeghost_core STATIC src/core/watcher.cpp src/core/indexer.cpp src/core/storage.cpp src/core/searcher.cpp) # creates a static library from the listed sources
target_include_directories(codeghost_core PUBLIC include src)                                                           # adds paths to header files
add_executable(codeghost src/cli/main.cpp)                                                                              # creates an executable file from main
target_link_libraries(codeghost PRIVATE codeghost_core)                                                                 # links the library to the executable file

if (UNIX AND NOT APPLE)
    target_link_libraries(codeghost_core PRIVATE pthread) # links a stream library to a static library
endif()
