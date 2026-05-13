#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

inline constexpr std::string_view BLUE = "\x1b[94m";
inline constexpr std::string_view YELLOW = "\x1b[33m";
inline constexpr std::string_view RED = "\x1b[31m";
inline constexpr std::string_view CLR = "\x1b[0m";

struct Config {
    std::vector<std::string> watch_paths;
    std::vector<std::string> ignore_paths;
    std::vector<std::string> critical_paths;
    int start_hour;
    int end_hour;
    bool watch_recursive = true;
};

void daemonise(bool silent = true);
Config loadFromConfig(const std::string& path);
void uploadToConfig(const Config& conf, const std::string& path);

