cmake_minimum_required(VERSION 3.20)

project(CodeGhost LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(codeghost_core STATIC 
    Application/application.cpp
    Inspector/scanner.cpp
    Inspector/hasher.cpp
    Integrity/integrity.cpp
    Storage/baselineStorage.cpp
    Settings/configLoader.cpp
    Resources/info.cpp
    Reporting/consoleReporter.cpp
)

target_compile_features(codeghost_core PUBLIC cxx_std_20)

target_include_directories(codeghost_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(codeghost_core PUBLIC OpenSSL::Crypto nlohmann_json::nlohmann_json)

add_executable(codeghost main.cpp)

target_link_libraries(codeghost PRIVATE codeghost_core)


