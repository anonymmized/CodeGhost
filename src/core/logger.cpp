#include "./logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <syslog.h>
#include <utility>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

Logger::Logger(const std::string& _path,
               uint8_t _log_level,
               uint8_t _tty_level,
               bool _colored,
               bool _timestamp,
               bool _server_logging,
               std::string _serverIp,
	       std::string _serverPort)
    : log_level(_log_level & 0x03),
      tty_level(_tty_level & 0x03),
      colored(_colored ? 1u : 0u),
      timestamp(_timestamp ? 1u : 0u),
      server_logging(_server_logging ? 1u : 0u),
      reserved(0),
      path(_path),
      serverIp(std::move(_serverIp)),
      serverPort(std::move(_serverPort)),
      file(_path, std::ios::app | std::ios::out) {
    if (!file.is_open()) {
        openlog("codeghost", LOG_PID | LOG_CONS, LOG_DAEMON);
        syslog(LOG_ERR, "Failed to open logfile: %s", path.c_str());
        //closelog();
        throw std::runtime_error("Failed to open logfile: " + path);
    }
    if (server_logging && !serverIp.empty()) {
        int port = -1;
        try {
            port = std::stoi(serverPort);
        } catch (const std::exception&) {
            port = -1;
        }
        if (port < 1 || port > 65535) {
            syslog(LOG_ERR, "Invalid port: %s", serverPort.c_str());
            server_logging = 0;
            closelog();
            return;
        }

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            syslog(LOG_ERR, "Failed to initialize socket");
            server_logging = 0;
            closelog();
            return;
        }

        dest.sin_family = AF_INET;
        dest.sin_port = htons(static_cast<uint16_t>(port));
        if (inet_pton(AF_INET, serverIp.c_str(), &dest.sin_addr) != 1) {
            syslog(LOG_ERR, "Invalid IP address: %s", serverIp.c_str());
            close(sock);
            sock = -1;
            server_logging = 0;
            closelog();
            return;
        }
        syslog(LOG_INFO, "Socket is open on %s:%s", serverIp.c_str(), serverPort.c_str());
    }
    closelog();
}

Logger::~Logger() {
    if (sock >= 0) {
        close(sock);
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

    if (server_logging && !serverIp.empty()) {
        std::string msg;
        if (timestamp) {
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d.%m.%y %H:%M:%S");
            msg += oss.str();
        }
        msg += strLevels[lvl];
        msg += str;
        ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0,
                              reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        if (sent < 0) {
            syslog(LOG_ERR, "UDP send failed");
        }
    }
}
