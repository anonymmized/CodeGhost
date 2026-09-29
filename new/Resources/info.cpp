#include "Resources/info.hpp"

#include <fstream>
#include <string>
#include <iostream>

void printHelp() {
    std::ifstream helpage("Resources/help.txt");
    if (!helpage.is_open()) {
        std::cout << "Error: there is no help page\n";
    }
    std::string line;
    while (std::getline(helpage, line)) {
        std::cout << line << '\n';
    }
}

void printVersion() {
    std::ifstream versionpage("Resources/version.txt");
    if (!versionpage.is_open()) {
        std::cout << "Error: there is no version page\n";
    }
    std::string line;
    std::vector<std::string> options;
    while (std::getline(versionpage, line)) {
        size_t equals = line.find('=');
        if (equals != std::string::npos) {
            std::cout << line.substr(0, equals) << ": " << line.substr(equals + 1) << '\n';
        }
        options.push_back(line);
    }
}
