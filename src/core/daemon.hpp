#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>

inline constexpr std::string_view BLUE = "\x1b[94m";
inline constexpr std::string_view YELLOW = "\x1b[33m";
inline constexpr std::string_view RED = "\x1b[31m";
inline constexpr std::string_view CLR = "\x1b[0m";

enum LOG_LEVEL {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

constexpr std::array<std::string_view, 3> strLevels {
    " [INFO] ",
    " [WARN] ",
    " [ERROR] "
};

constexpr std::array<std::string_view, 3> LOG_COLORS = {
    BLUE,
    YELLOW,
    RED
};

class LogSettings {
public:
    uint8_t log_level : 2;
    uint8_t tty_level : 2;
    uint8_t colored   : 1;
    uint8_t timestamp : 1;
    uint8_t reserved  : 2;
    constexpr LogSettings(uint8_t _log_level = LOG_INFO,
                          uint8_t _tty_level = LOG_INFO,
                          bool _colored = true,
                          bool _timestamp = true) noexcept
        : log_level(_log_level & 0x03),
          tty_level(_tty_level & 0x03),
          colored(_colored ? 1u : 0u),
          timestamp(_timestamp ? 1u : 0u),
          reserved(0)
    {}
};

struct Config {
    std::vector<std::string> watch_paths;
    std::vector<std::string> ignore_paths;
    std::vector<std::string> critical_paths;
    std::string logpath = "log.log";
    int start_hour;
    int end_hour;
    bool watch_recursive = true;
};

void daemonise(bool silent = true);
Config loadFromConfig(const std::string& path);
void uploadToConfig(const Config& conf);
void log(LOG_LEVEL level, const std::string& str, const LogSettings& settings, std::ostream& logfile);

