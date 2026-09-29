#pragma once

#include <vector>
#include <chrono>
#include <filesystem>

struct Config {
    bool recursive = false;
    std::vector<std::filesystem::path> ignorePaths;
    std::chrono::seconds scanInterval{300};
};
