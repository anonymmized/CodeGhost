#include "Resources/info.hpp"

#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>

void printHelp() {
    std::ifstream helpage("Resources/help.txt");
    if (!helpage.is_open()) {
        throw std::runtime_error("Cannot open Resources/help.txt");
    }
    std::string line;
    while (std::getline(helpage, line)) {
        std::cout << line << '\n';
    }
    if (helpage.bad()) {
        throw std::runtime_error("Failed to read help page");
    }
}

void printVersion() {
    std::ifstream versionpage("Resources/version.txt");
    if (!versionpage.is_open()) {
        throw std::runtime_error("Cannot open Resources/version.txt");
    }
    std::string line;
    while (std::getline(versionpage, line)) {
        size_t equals = line.find('=');
        if (equals != std::string::npos) {
            std::cout << line.substr(0, equals) << ": " << line.substr(equals + 1) << '\n';
        }
    }
    if (versionpage.bad()) {
        throw std::runtime_error("Failed to read version page");
    }
}
