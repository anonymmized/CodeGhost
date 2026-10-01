#pragma once

#include <vector>
#include <chrono>
#include <cstdint>
#include <filesystem>

struct Config {
    bool recursive = false;
    std::vector<std::filesystem::path> ignorePaths;
    std::uint64_t scanInterval = 300;
};
