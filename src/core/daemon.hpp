#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

struct Config {
    std::vector<std::string> watch_paths;
    std::vector<std::string> ignore_paths;
    std::vector<std::string> critical_paths;
    int start_hour;
    int end_hour;
    bool watch_recursive = true;
    std::string server_ip;
    std::string server_port = "10101";
};

void daemonise(bool silent = true);
Config loadFromConfig(const std::string& path);
void uploadToConfig(const Config& conf, const std::string& path);

