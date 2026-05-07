#pragma once

#include <string>
#include <cstdint>
#include <array>

class Logger {
    private:
        uint8_t log_level : 2;
        uint8_t tty_level : 2;
        uint8_t colored   : 1;
        uint8_t timestamp : 1;
        uint8_t reserved  : 2;
        LogLevel level;
        std::string path;
    public:
        uint8_t getLogLevel() { return log_level; }
        uint8_t getTtyLevel() { return tty_level; }
        uint8_t getColored() { return colred; }
        uint8_t getTimestamp() { return timestamp; }
        uint8_t getReserved() { return reserved; }
        constexpr Logger(uint8_t _log_level = LOG_INFO,
                         uint8_t _tty_level = LOG_INFO,
                         bool _colored = true,
                         bool _timestamp = true,
                         LogLevel _level,
                         std::string _path) noexcept
            : log_level(_log_level & 0x03),
              tty_level(_tty_level & 0x03),
              colored(_colored ? 1u : 0u),
              timestamp(_timestamp ? 1u : 0u),
              reserved(0),
              level(_level),
              path(_path)

        {}
        void log(const std::string& str);
        int checkStream(std::ofstream& file);
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



//void log(LogLevel level, const std::string& str, const LogSettings& settings, std::ostream& logfile);
