#include "logger.hpp"
#include "daemon.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>

void log(LogLevel level, const std::string& str, const LogSettings& settings, std::ostream& logfile) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    uint32_t lvl = static_cast<uint32_t>(level);
    if (level >= settings.log_level)
        logfile << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << str << '\n';

    if (level >= settings.tty_level) {
        if (settings.colored)
            std::cout << LOG_COLORS[lvl] << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << CLR << str << '\n';
        else
            std::cout << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << str << '\n';
    }
}
