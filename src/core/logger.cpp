#include "logger.hpp"
#include "daemon.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>

int checkStream(std::ofstream& file) {
    if (!file.is_open()) {
        openlog("daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
        syslog(LOG_ERR, "Failed to open logfile: %s", path.c_str());
        std::cerr << "Failed to open logfile: " << path << '\n';
        closelog();
        return 1;
    }
    return 0;
}

void Logger::log(const std::string& str) {
    std::ofstream file(path, std::ios::app | std::ios::out);
    if (checkStream(file)) {
        throw std::runtime_error("The logfile was not opened");
    }
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    uint32_t lvl = static_cast<uint32_t>(level);
    if (level >= log_level)
        path << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << str << '\n';

    if (level >= tty_level) {
        if (colored)
            std::cout << LOG_COLORS[lvl] << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << CLR << str << '\n';
        else
            std::cout << std::put_time(&tm, "%d.%m.%y %H:%M:%S") << strLevels[lvl] << str << '\n';
    }
}
