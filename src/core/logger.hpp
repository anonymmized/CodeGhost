#pragma once

#include <iomanip>
#include <chrono>
#include <array>

enum LogLevel {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

constexpr std::array<std::string_view, 3> strLevels {
    " [INFO] ",
    " [WARN] ",
    " [ERROR]"
};

constexpr std::array<std::string_view, 3> LogColors = {
    BLUE,
    YELLOW,
    RED
};



void log(LogLevel)
