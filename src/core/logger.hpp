#pragma once

#include <string>
#include <cstdint>
#include <array>
#include <fstream>

#include "daemon.hpp"

enum LogLevel {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

inline constexpr std::array<std::string_view, 3> strLevels {
    " [INFO] ",
    " [WARN] ",
    " [ERROR] "
};

inline constexpr std::array<std::string_view, 3> LOG_COLORS = {
    BLUE,
    YELLOW,
    RED
};

class Logger {
    private:
        uint8_t log_level : 2;
        uint8_t tty_level : 2;
        uint8_t colored   : 1;
        uint8_t timestamp : 1;
        uint8_t reserved  : 2;
        LogLevel level;

        std::string path;
        std::ofstream file;
    public:
        uint8_t getLogLevel() { return log_level; }
        uint8_t getTtyLevel() { return tty_level; }
        uint8_t getColored() { return colored; }
        uint8_t getTimestamp() { return timestamp; }
        uint8_t getReserved() { return reserved; }
        Logger(const std::string& _path,
                         uint8_t _log_level = LOG_INFO,
                         uint8_t _tty_level = LOG_INFO,
                         bool _colored = true,
                         bool _timestamp = true);
        void log(LogLevel level, const std::string& str);
};
