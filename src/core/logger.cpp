#include "./logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <syslog.h>
#include <utility>

Logger::Logger(const std::string& _path,
               uint8_t _log_level,
               uint8_t _tty_level,
               bool _colored,
               bool _timestamp,
               bool _server_logging,
               std::string _login_path)
    : log_level(_log_level & 0x03),
      tty_level(_tty_level & 0x03),
      colored(_colored ? 1u : 0u),
      timestamp(_timestamp ? 1u : 0u),
      server_logging(_server_logging ? 1u : 0u),
      reserved(0),
      path(_path),
      login_path(std::move(_login_path)),
      file(_path, std::ios::app | std::ios::out) {
    if (!file.is_open()) {
        openlog("codeghost", LOG_PID | LOG_CONS, LOG_DAEMON);
        syslog(LOG_ERR, "Failed to open logfile: %s", path.c_str());
        closelog();
        throw std::runtime_error("Failed to open logfile: " + path);
    }
}

void Logger::log(LogLevel level, const std::string& str) {
    if (level < log_level) return;

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    uint32_t lvl = static_cast<uint32_t>(level);
    if (level >= log_level) {
        if (timestamp) {
            file << std::put_time(&tm, "%d.%m.%y %H:%M:%S");
        }
        file << strLevels[lvl] << str << '\n';
        file.flush();
    }

    if (level >= tty_level) {
        if (colored) {
            std::cout << LOG_COLORS[lvl];
        }
        if (timestamp) {
            std::cout << std::put_time(&tm, "%d.%m.%y %H:%M:%S");
        }
        std::cout << strLevels[lvl] << str;
        if (colored) {
            std::cout << runtime::CLR;
        }
        std::cout << '\n';
    }

    if (server_logging && !login_path.empty()) {
        // The credentials path is now carried through logger settings for future remote transport integration.
    }
}
