#pragma once

#include <string>
#include <cstdint>
#include <array>

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



void log(LogLevel level, const std::string& str, const LogSettings& settings, std::ostream& logfile);
